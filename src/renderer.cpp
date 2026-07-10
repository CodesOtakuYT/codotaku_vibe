#include <renderer.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <uploader.hpp>
#include <scene.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <format>
#include <span>

#include <entt/entity/registry.hpp>
#include <components.hpp>

namespace {

auto ChooseSampleCount(SDL_GPUDevice *device, SDL_GPUTextureFormat format, Renderer::MsaaMode mode) -> SDL_GPUSampleCount {
    if (mode == Renderer::MsaaMode::None) return SDL_GPU_SAMPLECOUNT_1;
    auto desired = static_cast<SDL_GPUSampleCount>(static_cast<int>(mode));
    return SDL_GPUTextureSupportsSampleCount(device, format, desired) ? desired : SDL_GPU_SAMPLECOUNT_1;
}

} // namespace

Renderer::Renderer(GPUContext *gpu, ResourceManager *resources, Uploader &uploader, Scene &scene, MsaaMode msaa)
    : gpu_(gpu)
    , resources_(resources)
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
    SDL_GPUShader *frag = resources->LoadShader("Cube.frag", 1);

    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(PositionTextureVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };

    SDL_GPUVertexAttribute attrs[2] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(PositionTextureVertex, uv) },
    };

    SDL_GPUVertexInputState vertexInput = {
        .vertex_buffer_descriptions = &vbDesc,
        .num_vertex_buffers = 1,
        .vertex_attributes = attrs,
        .num_vertex_attributes = 2,
    };

    uploader_.Begin();

    auto texHandle = resources->LoadTexture("ravioli.bmp");

    materials_.emplace_back(
        device,
        MaterialCreateInfo{
            .vertex_shader = vert,
            .fragment_shader = frag,
            .texture = texHandle.texture,
            .sampler = texHandle.sampler,
            .color_format = gpu_->SwapchainFormat(),
            .sample_count = sampleCount,
        },
        vertexInput
    );

    // Wireframe material (no texture, green, line fill)
    SDL_GPUShader *solidFrag = resources->LoadShader("SolidColor.frag", 0, 0);
    materials_.emplace_back(
        device,
        MaterialCreateInfo{
            .vertex_shader = vert,
            .fragment_shader = solidFrag,
            .texture = nullptr,
            .sampler = nullptr,
            .color_format = gpu_->SwapchainFormat(),
            .sample_count = sampleCount,
            .fill_mode = SDL_GPU_FILLMODE_LINE,
        },
        vertexInput
    );
    SDL_ReleaseGPUShader(device, solidFrag);

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    for (const auto &geo : scene.Geometries()) {
        auto &gb = geometry_buffers_.emplace_back();
        gb.index_count = static_cast<int>(geo.indices.size());

        SDL_GPUBufferCreateInfo vbInfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = static_cast<Uint32>(geo.vertices.size() * sizeof(PositionTextureVertex)),
        };
        gb.vertex_buffer = chk(SDL_CreateGPUBuffer(device, &vbInfo));
        uploader_.Buffer(gb.vertex_buffer, 0, std::as_bytes(std::span(geo.vertices)));

        SDL_GPUBufferCreateInfo ibInfo = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = static_cast<Uint32>(geo.indices.size() * sizeof(Uint32)),
        };
        gb.index_buffer = chk(SDL_CreateGPUBuffer(device, &ibInfo));
        gb.index_size = SDL_GPU_INDEXELEMENTSIZE_32BIT;
        uploader_.Buffer(gb.index_buffer, 0, std::as_bytes(std::span(geo.indices)));
    }
    uploader_.End();
}

Renderer::~Renderer() {
    auto device = gpu_->Device();
    for (auto &gb : geometry_buffers_) {
        SDL_ReleaseGPUBuffer(device, gb.vertex_buffer);
        SDL_ReleaseGPUBuffer(device, gb.index_buffer);
    }
}

void Renderer::Resize(glm::ivec2 size) {
    size_ = size;
    gbuffer_.Resize(size);
}

auto Renderer::AddMaterial(TextureHandle texture) -> size_t {
    auto device = gpu_->Device();
    SDL_GPUShader *vert = resources_->LoadShader("Cube.vert", 0, 1);
    SDL_GPUShader *frag = resources_->LoadShader("Cube.frag", 1);

    auto sampleCount = ChooseSampleCount(device, gpu_->SwapchainFormat(), msaa_);

    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(PositionTextureVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };

    SDL_GPUVertexAttribute attrs[2] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = offsetof(PositionTextureVertex, uv) },
    };

    SDL_GPUVertexInputState vertexInput = {
        .vertex_buffer_descriptions = &vbDesc,
        .num_vertex_buffers = 1,
        .vertex_attributes = attrs,
        .num_vertex_attributes = 2,
    };

    materials_.emplace_back(
        device,
        MaterialCreateInfo{
            .vertex_shader = vert,
            .fragment_shader = frag,
            .texture = texture.texture,
            .sampler = texture.sampler,
            .color_format = gpu_->SwapchainFormat(),
            .sample_count = sampleCount,
        },
        vertexInput
    );

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    return materials_.size() - 1;
}

void Renderer::Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, const glm::mat4 &viewProj, entt::registry &scene) {
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

    int last_mat = -1;
    auto view = scene.view<const GeometryRef, const MaterialRef, const Transform>();
    for (auto entity : view) {
        auto [geoRef, matRef, xform] = view.get(entity);

        if (static_cast<int>(matRef.index) != last_mat) {
            materials_[matRef.index].Bind(pass, cmdbuf);
            last_mat = static_cast<int>(matRef.index);
        }

        auto &gb = geometry_buffers_[geoRef.index];

        SDL_GPUBufferBinding vbBind = { .buffer = gb.vertex_buffer };
        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

        SDL_GPUBufferBinding ibBind = { .buffer = gb.index_buffer };
        SDL_BindGPUIndexBuffer(pass, &ibBind, gb.index_size);

        glm::mat4 mvp = viewProj * xform.value;
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
