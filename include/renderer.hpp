#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

#include <gbuffer.hpp>

class GPUContext;
class ResourceManager;
class OrbitCamera;
class Scene;

class Renderer {
public:
    enum class MsaaMode { None, X2, X4, X8 };

    Renderer(GPUContext *gpu, ResourceManager *resources, MsaaMode msaa = MsaaMode::None);
    ~Renderer();

    void Resize(int w, int h);
    void Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, int w, int h, const OrbitCamera &camera, Scene &scene);

private:
    GPUContext *gpu_;
    GBuffer gbuffer_;
    SDL_GPUGraphicsPipeline *pipeline_ = nullptr;
    MsaaMode msaa_ = MsaaMode::None;
};
