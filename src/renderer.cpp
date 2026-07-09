#include <renderer.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <uploader.hpp>
#include <scene.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <span>

namespace {

auto ChooseSampleCount(SDL_GPUDevice *device, SDL_GPUTextureFormat format, Renderer::MsaaMode mode) -> SDL_GPUSampleCount {
    if (mode == Renderer::MsaaMode::None) return SDL_GPU_SAMPLECOUNT_1;
    auto desired = static_cast<SDL_GPUSampleCount>(static_cast<int>(mode));
    return SDL_GPUTextureSupportsSampleCount(device, format, desired) ? desired : SDL_GPU_SAMPLECOUNT_1;
}

} // namespace

Renderer::Renderer(GPUContext *gpu, ResourceManager *resources, Uploader &uploader, Scene &scene, MsaaMode msaa)
    : gpu_(gpu)
    , gbuffer_(gpu->Device(), gpu->Size())
    , uploader_(uploader)
    , msaa_(msaa)
    , size_(gpu->Size()) {

    auto device = gpu_->Device();
    auto sampleCount = ChooseSampleCount(device, gpu_->SwapchainFormat(), msaa);

    if (msaa_ == MsaaMode::None) {
        depth_att_ = gbuffer_.AddAttachment({
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        });
    } else {
        color_att_ = gbuffer_.AddAttachment({
            .format = gpu_->SwapchainFormat(),
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            .sample_count = sampleCount,
        });
        resolve_att_ = gbuffer_.AddAttachment({
            .format = gpu_->SwapchainFormat(),
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        });
        depth_att_ = gbuffer_.AddAttachment({
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .sample_count = sampleCount,
        });
    }

    SDL_GPUShader *vert = resources->LoadShader("Cube.vert", 0, 1);
    SDL_GPUShader *frag = resources->LoadShader("Cube.frag");

    SDL_GPUColorTargetDescription colorDesc = {
        .format = gpu_->SwapchainFormat(),
    };

    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(PositionColorVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };

    SDL_GPUVertexAttribute attrs[2] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, .offset = offsetof(PositionColorVertex, color) },
    };

    SDL_GPUVertexInputState vertexInput = {
        .vertex_buffer_descriptions = &vbDesc,
        .num_vertex_buffers = 1,
        .vertex_attributes = attrs,
        .num_vertex_attributes = 2,
    };

    SDL_GPUDepthStencilState depthState = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    SDL_GPURasterizerState rasterState = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_BACK,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };

    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {
        .color_target_descriptions = &colorDesc,
        .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .has_depth_stencil_target = true,
    };

    SDL_GPUMultisampleState multisampleState = {
        .sample_count = sampleCount,
    };

    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = vertexInput,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterState,
        .multisample_state = multisampleState,
        .depth_stencil_state = depthState,
        .target_info = targetInfo,
    };

    pipeline_ = chk(SDL_CreateGPUGraphicsPipeline(device, &info));

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    uploader_.Begin();
    for (const auto &geo : scene.Geometries()) {
        auto &gb = geometry_buffers_.emplace_back();
        gb.index_count = static_cast<int>(geo.indices.size());

        SDL_GPUBufferCreateInfo vbInfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = static_cast<Uint32>(geo.vertices.size() * sizeof(PositionColorVertex)),
        };
        gb.vertex_buffer = chk(SDL_CreateGPUBuffer(device, &vbInfo));
        uploader_.Buffer(gb.vertex_buffer, 0, std::as_bytes(std::span(geo.vertices)));

        SDL_GPUBufferCreateInfo ibInfo = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = static_cast<Uint32>(geo.indices.size() * sizeof(Uint16)),
        };
        gb.index_buffer = chk(SDL_CreateGPUBuffer(device, &ibInfo));
        uploader_.Buffer(gb.index_buffer, 0, std::as_bytes(std::span(geo.indices)));
    }
    uploader_.End();
}

Renderer::~Renderer() {
    auto device = gpu_->Device();
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_);
    for (auto &gb : geometry_buffers_) {
        SDL_ReleaseGPUBuffer(device, gb.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, gb.index_buffer);
    }
}

void Renderer::Resize(glm::ivec2 size) {
    size_ = size;
    gbuffer_.Resize(size);
}

void Renderer::Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, const glm::mat4 &viewProj, Scene &scene) {
    SDL_GPUColorTargetInfo colorInfo = {
        .clear_color = SDL_FColor{ 0.1f, 0.1f, 0.2f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
    };

    if (msaa_ == MsaaMode::None) {
        colorInfo.texture = swapchain;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    } else {
        colorInfo.texture = gbuffer_.GetTexture(color_att_);
        colorInfo.store_op = SDL_GPU_STOREOP_RESOLVE;
        colorInfo.resolve_texture = gbuffer_.GetTexture(resolve_att_);
    }

    SDL_GPUDepthStencilTargetInfo depthInfo = {
        .texture = gbuffer_.GetTexture(depth_att_),
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
    };

    SDL_GPURenderPass *pass = chk(SDL_BeginGPURenderPass(cmdbuf, &colorInfo, 1, &depthInfo));

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);

    for (const auto &inst : scene.Instances()) {
        auto &gb = geometry_buffers_[inst.geometry_index];

        SDL_GPUBufferBinding vbBind = { .buffer = gb.vertex_buffer };
        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

        SDL_GPUBufferBinding ibBind = { .buffer = gb.index_buffer };
        SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        glm::mat4 mvp = viewProj * inst.Transform();
        SDL_PushGPUVertexUniformData(cmdbuf, 0, glm::value_ptr(mvp), sizeof(glm::mat4));
        SDL_DrawGPUIndexedPrimitives(pass, gb.index_count, 1, 0, 0, 0);
    }

    SDL_EndGPURenderPass(pass);

    if (msaa_ != MsaaMode::None) {
        SDL_GPUBlitInfo blitInfo = {
            .source = {
                .texture = gbuffer_.GetTexture(resolve_att_),
                .w = static_cast<Uint32>(size_.x),
                .h = static_cast<Uint32>(size_.y),
            },
            .destination = {
                .texture = swapchain,
                .w = static_cast<Uint32>(size_.x),
                .h = static_cast<Uint32>(size_.y),
            },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .filter = SDL_GPU_FILTER_LINEAR,
        };
        SDL_BlitGPUTexture(cmdbuf, &blitInfo);
    }
}
