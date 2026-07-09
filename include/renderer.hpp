#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

#include <gbuffer.hpp>

class GPUContext;
class ResourceManager;
class Uploader;
class Scene;

struct PositionColorVertex {
    glm::vec3 pos;
    glm::u8vec4 color;
};

class Renderer {
public:
    enum class MsaaMode { None, X2, X4, X8 };

    Renderer(GPUContext *gpu, ResourceManager *resources, Uploader &uploader, MsaaMode msaa = MsaaMode::None);
    ~Renderer();

    void Resize(glm::ivec2 size);
    void Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, const glm::mat4 &viewProj, Scene &scene);

private:
    GPUContext *gpu_;
    GBuffer gbuffer_;
    Uploader &uploader_;
    SDL_GPUGraphicsPipeline *pipeline_ = nullptr;
    MsaaMode msaa_ = MsaaMode::None;
    glm::ivec2 size_{};
    int color_att_ = -1;
    int resolve_att_ = -1;
    int depth_att_ = -1;
    SDL_GPUBuffer *vertex_buffer_ = nullptr;
    SDL_GPUBuffer *index_buffer_ = nullptr;
    int index_count_ = 0;
};
