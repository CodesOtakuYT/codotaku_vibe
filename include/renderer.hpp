#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

#include <gbuffer.hpp>
#include <material.hpp>

#include <vector>

class GPUContext;
class ResourceManager;
class Uploader;
class Scene;

class Renderer {
public:
    enum class MsaaMode { None, X2, X4, X8 };

    Renderer(GPUContext *gpu, ResourceManager *resources, Uploader &uploader, Scene &scene, MsaaMode msaa = MsaaMode::None);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void Resize(glm::ivec2 size);
    void Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, const glm::mat4 &viewProj, Scene &scene);

private:
    struct GeometryBuffers {
        SDL_GPUBuffer *vertex_buffer = nullptr;
        SDL_GPUBuffer *index_buffer = nullptr;
        int index_count = 0;
    };

    GPUContext *gpu_;
    GBuffer gbuffer_;
    Uploader &uploader_;
    std::vector<Material> materials_;
    MsaaMode msaa_ = MsaaMode::None;
    glm::ivec2 size_{};
    int color_att_ = -1;
    int resolve_att_ = -1;
    int depth_att_ = -1;
    std::vector<GeometryBuffers> geometry_buffers_;
};
