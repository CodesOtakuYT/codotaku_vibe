#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

class GPUContext;
class GBuffer;
class ResourceManager;

struct PositionColorVertex {
    glm::vec3 pos;
    Uint8 r, g, b, a;
};

class Renderer {
public:
    Renderer(GPUContext *gpu, GBuffer *gbuffer, ResourceManager *resources);
    ~Renderer();

    void Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, int w, int h, float dt);
    void Event(const SDL_Event &event);

private:
    void CreateCubeResources();

    GPUContext *gpu_;
    GBuffer *gbuffer_;
    ResourceManager *resources_;

    SDL_GPUGraphicsPipeline *pipeline_ = nullptr;
    SDL_GPUBuffer *vertex_buffer_ = nullptr;
    SDL_GPUBuffer *index_buffer_ = nullptr;

    glm::vec2 last_mouse_{};
    bool mouse_down_ = false;
    float pitch_ = 0.3f;
    float yaw_ = 0.0f;
    float distance_ = 30.0f;
};
