#pragma once

#include <sdl.hpp>

struct MaterialCreateInfo {
    SDL_GPUShader *vertex_shader;
    SDL_GPUShader *fragment_shader;
    SDL_GPUTexture *texture;
    SDL_GPUSampler *sampler;
    SDL_GPUTextureFormat color_format;
    SDL_GPUTextureFormat depth_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUFillMode fill_mode = SDL_GPU_FILLMODE_FILL;
    SDL_GPUCullMode cull_mode = SDL_GPU_CULLMODE_BACK;
    SDL_GPUFrontFace front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    SDL_GPUCompareOp compare_op = SDL_GPU_COMPAREOP_LESS;
    bool depth_test = true;
    bool depth_write = true;
    SDL_GPUPrimitiveType primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
};

class Material {
public:
    Material(SDL_GPUDevice *device, const MaterialCreateInfo &info, const SDL_GPUVertexInputState &vertex_input);
    ~Material();

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&& other) noexcept;
    Material& operator=(Material&& other) noexcept;

    void Bind(SDL_GPURenderPass *pass, SDL_GPUCommandBuffer *cmdbuf) const;

    SDL_GPUGraphicsPipeline *Pipeline() const { return pipeline_; }

private:
    SDL_GPUDevice *device_ = nullptr;
    SDL_GPUGraphicsPipeline *pipeline_ = nullptr;
    SDL_GPUTexture *texture_ = nullptr;
    SDL_GPUSampler *sampler_ = nullptr;
};
