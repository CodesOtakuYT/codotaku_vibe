#include <material.hpp>

Material::Material(SDL_GPUDevice *device, const MaterialCreateInfo &info, const SDL_GPUVertexInputState &vertex_input)
    : device_(device)
    , texture_(info.texture)
    , sampler_(info.sampler) {

    SDL_GPUColorTargetDescription colorDesc = { .format = info.color_format };

    SDL_GPUDepthStencilState depthState = {
        .compare_op = info.compare_op,
        .enable_depth_test = info.depth_test,
        .enable_depth_write = info.depth_write,
    };

    SDL_GPURasterizerState rasterState = {
        .fill_mode = info.fill_mode,
        .cull_mode = info.cull_mode,
        .front_face = info.front_face,
    };

    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {
        .color_target_descriptions = &colorDesc,
        .num_color_targets = 1,
        .depth_stencil_format = info.depth_format,
        .has_depth_stencil_target = true,
    };

    SDL_GPUMultisampleState multisampleState = { .sample_count = info.sample_count };

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
        .vertex_shader = info.vertex_shader,
        .fragment_shader = info.fragment_shader,
        .vertex_input_state = vertex_input,
        .primitive_type = info.primitive_type,
        .rasterizer_state = rasterState,
        .multisample_state = multisampleState,
        .depth_stencil_state = depthState,
        .target_info = targetInfo,
    };

    pipeline_ = chk(SDL_CreateGPUGraphicsPipeline(device_, &pipelineInfo));
}

Material::Material(Material &&other) noexcept
    : device_(other.device_)
    , pipeline_(other.pipeline_)
    , texture_(other.texture_)
    , sampler_(other.sampler_) {
    other.device_ = nullptr;
    other.pipeline_ = nullptr;
    other.texture_ = nullptr;
    other.sampler_ = nullptr;
}

Material &Material::operator=(Material &&other) noexcept {
    if (this != &other) {
        if (pipeline_) SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
        if (texture_) SDL_ReleaseGPUTexture(device_, texture_);
        if (sampler_) SDL_ReleaseGPUSampler(device_, sampler_);
        device_ = other.device_;
        pipeline_ = other.pipeline_;
        texture_ = other.texture_;
        sampler_ = other.sampler_;
        other.device_ = nullptr;
        other.pipeline_ = nullptr;
        other.texture_ = nullptr;
        other.sampler_ = nullptr;
    }
    return *this;
}

Material::~Material() {
    if (pipeline_) SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
    if (texture_) SDL_ReleaseGPUTexture(device_, texture_);
    if (sampler_) SDL_ReleaseGPUSampler(device_, sampler_);
}

void Material::Bind(SDL_GPURenderPass *pass, SDL_GPUCommandBuffer *cmdbuf) const {
    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    SDL_GPUTextureSamplerBinding texBind = { .texture = texture_, .sampler = sampler_ };
    SDL_BindGPUFragmentSamplers(pass, 0, &texBind, 1);
}
