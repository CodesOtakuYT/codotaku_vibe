#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

class Uploader;

struct PositionColorVertex {
    glm::vec3 pos;
    Uint8 r, g, b, a;
};

class Scene {
public:
    Scene(SDL_GPUDevice *device, Uploader &uploader);
    ~Scene();

    void Update(float dt);
    void Render(SDL_GPURenderPass *pass, SDL_GPUCommandBuffer *cmdbuf, const glm::mat4 &viewProj);

private:
    SDL_GPUDevice *device_;
    SDL_GPUBuffer *vertex_buffer_ = nullptr;
    SDL_GPUBuffer *index_buffer_ = nullptr;
    int index_count_ = 0;
    float rotation_ = 0.0f;
};
