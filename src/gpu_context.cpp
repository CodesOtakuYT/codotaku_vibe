#include <gpu_context.hpp>

GPUContext::GPUContext(const char *title, glm::ivec2 size)
    : size_(size) {
    chk(SDL_SetAppMetadata(title, "0.0.1", "com.codotaku.engine"));
    chk(SDL_Init(SDL_INIT_VIDEO));
    window_ = chk(SDL_CreateWindow(title, size.x, size.y, SDL_WINDOW_RESIZABLE));
    device_ = chk(SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true, nullptr));
    chk(SDL_ClaimWindowForGPUDevice(device_, window_));
    chk(SDL_ShowWindow(window_));
    SDL_GetWindowSizeInPixels(window_, &size_.x, &size_.y);
}

GPUContext::~GPUContext() {
    if (device_) {
        SDL_ReleaseWindowFromGPUDevice(device_, window_);
        SDL_DestroyGPUDevice(device_);
    }
    if (window_) SDL_DestroyWindow(window_);
}

SDL_GPUTextureFormat GPUContext::SwapchainFormat() const {
    return SDL_GetGPUSwapchainTextureFormat(device_, window_);
}

bool GPUContext::BeginFrame(SDL_GPUCommandBuffer *&cmdbuf, SDL_GPUTexture *&swapchain) {
    cmdbuf = chk(SDL_AcquireGPUCommandBuffer(device_));

    SDL_GPUTexture *sc = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window_, &sc, nullptr, nullptr)) {
        chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
        return false;
    }

    if (!sc) {
        chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
        return false;
    }

    swapchain = sc;

    int w, h;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    if (w != size_.x || h != size_.y) {
        size_.x = w;
        size_.y = h;
    }

    return true;
}

void GPUContext::EndFrame(SDL_GPUCommandBuffer *cmdbuf) {
    chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
}
