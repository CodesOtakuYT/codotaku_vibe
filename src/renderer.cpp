#include <renderer.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <uploader.hpp>
#include <scene.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <format>
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

    // Load texture
    const char *base = SDL_GetBasePath();
    std::string texPath = std::string(base) + "assets/textures/ravioli.bmp";
    SDL_Surface *surface = chk(SDL_LoadBMP(texPath.c_str()));

    SDL_GPUTextureCreateInfo texInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(surface->w),
        .height = static_cast<Uint32>(surface->h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture *texture = chk(SDL_CreateGPUTexture(device, &texInfo));

    SDL_GPUSamplerCreateInfo sampInfo = {
        .min_filter = SDL_GPU_FILTER_LINEAR,
        .mag_filter = SDL_GPU_FILTER_LINEAR,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    };
    SDL_GPUSampler *sampler = chk(SDL_CreateGPUSampler(device, &sampInfo));

    Uint32 texSize = static_cast<Uint32>(surface->w * surface->h * 4);
    SDL_GPUTransferBufferCreateInfo tbInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = texSize,
    };
    SDL_GPUTransferBuffer *tbuf = chk(SDL_CreateGPUTransferBuffer(device, &tbInfo));
    auto *mapped = static_cast<Uint8 *>(chk(SDL_MapGPUTransferBuffer(device, tbuf, false)));
    std::memcpy(mapped, surface->pixels, texSize);
    SDL_UnmapGPUTransferBuffer(device, tbuf);

    SDL_GPUCommandBuffer *cmd = chk(SDL_AcquireGPUCommandBuffer(device));
    SDL_GPUCopyPass *copy = chk(SDL_BeginGPUCopyPass(cmd));
    SDL_GPUTextureTransferInfo srcInfo = { .transfer_buffer = tbuf, .offset = 0 };
    SDL_GPUTextureRegion dstRegion = {
        .texture = texture,
        .w = static_cast<Uint32>(surface->w),
        .h = static_cast<Uint32>(surface->h),
        .d = 1,
    };
    SDL_UploadToGPUTexture(copy, &srcInfo, &dstRegion, false);
    SDL_EndGPUCopyPass(copy);
    chk(SDL_SubmitGPUCommandBuffer(cmd));

    SDL_ReleaseGPUTransferBuffer(device, tbuf);
    SDL_DestroySurface(surface);

    materials_.emplace_back(
        device,
        MaterialCreateInfo{
            .vertex_shader = vert,
            .fragment_shader = frag,
            .texture = texture,
            .sampler = sampler,
            .color_format = gpu_->SwapchainFormat(),
            .sample_count = sampleCount,
        },
        vertexInput
    );

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    uploader_.Begin();
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
            .size = static_cast<Uint32>(geo.indices.size() * sizeof(Uint16)),
        };
        gb.index_buffer = chk(SDL_CreateGPUBuffer(device, &ibInfo));
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

    for (const auto &inst : scene.Instances()) {
        materials_[inst.material_index].Bind(pass, cmdbuf);

        auto &gb = geometry_buffers_[inst.geometry_index];

        SDL_GPUBufferBinding vbBind = { .buffer = gb.vertex_buffer };
        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

        SDL_GPUBufferBinding ibBind = { .buffer = gb.index_buffer };
        SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        glm::mat4 mvp = viewProj * inst.transform;
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
