#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

class GPUContext {
public:
    GPUContext(const char *title, glm::ivec2 size);
    ~GPUContext();

    SDL_GPUDevice *Device() const { return device_; }
    glm::ivec2 Size() const { return size_; }
    SDL_GPUTextureFormat SwapchainFormat() const;

    bool BeginFrame(SDL_GPUCommandBuffer *&cmdbuf, SDL_GPUTexture *&swapchain);
    void EndFrame(SDL_GPUCommandBuffer *cmdbuf);

private:
    SDL_Window *window_ = nullptr;
    SDL_GPUDevice *device_ = nullptr;
    glm::ivec2 size_{};
};
