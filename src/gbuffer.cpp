#include <gbuffer.hpp>

GBuffer::GBuffer(SDL_GPUDevice *device, int width, int height)
    : device_(device) {
    SDL_GPUTextureCreateInfo dtInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = static_cast<Uint32>(width),
        .height = static_cast<Uint32>(height),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    depth_texture_ = chk(SDL_CreateGPUTexture(device_, &dtInfo));
}

GBuffer::~GBuffer() {
    if (depth_texture_) SDL_ReleaseGPUTexture(device_, depth_texture_);
}

void GBuffer::Resize(int width, int height) {
    SDL_ReleaseGPUTexture(device_, depth_texture_);

    SDL_GPUTextureCreateInfo dtInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = static_cast<Uint32>(width),
        .height = static_cast<Uint32>(height),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    depth_texture_ = chk(SDL_CreateGPUTexture(device_, &dtInfo));
}
