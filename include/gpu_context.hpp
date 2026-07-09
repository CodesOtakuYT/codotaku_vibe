#pragma once

#include <sdl.hpp>

class GPUContext {
public:
    GPUContext(const char *title, int width, int height);
    ~GPUContext();

    SDL_GPUDevice *Device() const { return device_; }
    int Width() const { return win_w_; }
    int Height() const { return win_h_; }
    SDL_GPUTextureFormat SwapchainFormat() const;

    bool BeginFrame(SDL_GPUCommandBuffer *&cmdbuf, SDL_GPUTexture *&swapchain);
    void EndFrame(SDL_GPUCommandBuffer *cmdbuf);

private:
    SDL_Window *window_ = nullptr;
    SDL_GPUDevice *device_ = nullptr;
    int win_w_ = 0;
    int win_h_ = 0;
};
