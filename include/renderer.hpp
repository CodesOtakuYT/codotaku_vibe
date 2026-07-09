#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>

#include <span>
#include <string>
#include <string_view>

struct PositionColorVertex {
    glm::vec3 pos;
    Uint8 r, g, b, a;
};

class Renderer {
public:
    Renderer(SDL_Window *window, SDL_GPUDevice *device);
    ~Renderer();

    void Render(float dt);
    void Event(const SDL_Event &event);

private:
    SDL_GPUShader *LoadShader(const char *filename, Uint32 samplerCount = 0, Uint32 uniformBufferCount = 0);
    void CreateCubeResources();
    void CreateDepthTexture();

    SDL_Window *window_ = nullptr;
    SDL_GPUDevice *device_ = nullptr;
    SDL_GPUGraphicsPipeline *pipeline_ = nullptr;
    SDL_GPUBuffer *vertex_buffer_ = nullptr;
    SDL_GPUBuffer *index_buffer_ = nullptr;
    SDL_GPUTexture *depth_texture_ = nullptr;

    std::string base_path_;
    std::string_view shader_dir_;
    std::string_view shader_ext_;
    const char *entrypoint_ = "main";

    glm::vec2 last_mouse_{};
    bool mouse_down_ = false;
    float pitch_ = 0.3f;
    float yaw_ = 0.0f;
    float distance_ = 30.0f;
    int win_w_ = 0;
    int win_h_ = 0;
};
