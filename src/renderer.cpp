#include <renderer.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <camera.hpp>
#include <scene.hpp>

#include <cstddef>

namespace {

auto ChooseSampleCount(SDL_GPUDevice *device, SDL_GPUTextureFormat format, Renderer::MsaaMode mode) -> SDL_GPUSampleCount {
    if (mode == Renderer::MsaaMode::None) return SDL_GPU_SAMPLECOUNT_1;
    auto desired = static_cast<SDL_GPUSampleCount>(static_cast<int>(mode));
    return SDL_GPUTextureSupportsSampleCount(device, format, desired) ? desired : SDL_GPU_SAMPLECOUNT_1;
}

} // namespace

Renderer::Renderer(GPUContext *gpu, ResourceManager *resources, MsaaMode msaa)
    : gpu_(gpu)
    , gbuffer_(gpu->Device(), gpu->Width(), gpu->Height())
    , msaa_(msaa) {

    auto device = gpu_->Device();

    auto sampleCount = ChooseSampleCount(device, gpu_->SwapchainFormat(), msaa);

    if (msaa != MsaaMode::None) {
        gbuffer_.AddAttachment({
            .format = gpu_->SwapchainFormat(),
            .type = AttachmentType::Color,
            .sample_count = sampleCount,
        });
    }
    gbuffer_.AddAttachment({
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .type = AttachmentType::DepthStencil,
        .sample_count = sampleCount,
    });

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
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, .offset = offsetof(PositionColorVertex, r) },
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
}

Renderer::~Renderer() {
    auto device = gpu_->Device();
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_);
}

void Renderer::Resize(int w, int h) {
    gbuffer_.Resize(w, h);
}

void Renderer::Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, int w, int h, const OrbitCamera &camera, Scene &scene) {
    auto viewProj = camera.ViewProjMatrix(w, h);

    SDL_GPUColorTargetInfo colorInfo = {
        .clear_color = SDL_FColor{ 0.1f, 0.1f, 0.2f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
    };

    if (msaa_ == MsaaMode::None) {
        colorInfo.texture = swapchain;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    } else {
        colorInfo.texture = gbuffer_.GetTexture(0);
        colorInfo.store_op = SDL_GPU_STOREOP_RESOLVE;
        colorInfo.resolve_texture = gbuffer_.GetResolveTexture(0);
    }

    SDL_GPUDepthStencilTargetInfo depthInfo = {
        .texture = gbuffer_.GetTexture(msaa_ == MsaaMode::None ? 0 : 1),
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
    };

    SDL_GPURenderPass *pass = chk(SDL_BeginGPURenderPass(cmdbuf, &colorInfo, 1, &depthInfo));

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    scene.Render(pass, cmdbuf, viewProj);

    SDL_EndGPURenderPass(pass);

    if (msaa_ != MsaaMode::None) {
        SDL_GPUBlitInfo blitInfo = {
            .source = {
                .texture = gbuffer_.GetResolveTexture(0),
                .w = static_cast<Uint32>(w),
                .h = static_cast<Uint32>(h),
            },
            .destination = {
                .texture = swapchain,
                .w = static_cast<Uint32>(w),
                .h = static_cast<Uint32>(h),
            },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .filter = SDL_GPU_FILTER_LINEAR,
        };
        SDL_BlitGPUTexture(cmdbuf, &blitInfo);
    }
}
