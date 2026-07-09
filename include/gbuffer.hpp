#pragma once

#include <sdl.hpp>

class GBuffer {
public:
    GBuffer(SDL_GPUDevice *device, int width, int height);
    ~GBuffer();

    void Resize(int width, int height);
    SDL_GPUTexture *DepthTexture() const { return depth_texture_; }

private:
    SDL_GPUDevice *device_ = nullptr;
    SDL_GPUTexture *depth_texture_ = nullptr;
};
