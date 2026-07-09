#include <renderer.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <uploader.hpp>
#include <scene.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <span>

namespace {

auto ChooseSampleCount(SDL_GPUDevice *device, SDL_GPUTextureFormat format, Renderer::MsaaMode mode) -> SDL_GPUSampleCount {
    if (mode == Renderer::MsaaMode::None) return SDL_GPU_SAMPLECOUNT_1;
    auto desired = static_cast<SDL_GPUSampleCount>(static_cast<int>(mode));
    return SDL_GPUTextureSupportsSampleCount(device, format, desired) ? desired : SDL_GPU_SAMPLECOUNT_1;
}

PositionColorVertex cubeVerts[24] = {
    { { -10, -10, -10 }, { 255,   0,   0, 255 } },
    { {  10, -10, -10 }, { 255,   0,   0, 255 } },
    { {  10,  10, -10 }, { 255,   0,   0, 255 } },
    { { -10,  10, -10 }, { 255,   0,   0, 255 } },
    { { -10, -10,  10 }, { 255, 255,   0, 255 } },
    { {  10, -10,  10 }, { 255, 255,   0, 255 } },
    { {  10,  10,  10 }, { 255, 255,   0, 255 } },
    { { -10,  10,  10 }, { 255, 255,   0, 255 } },
    { { -10, -10, -10 }, { 255,   0, 255, 255 } },
    { { -10,  10, -10 }, { 255,   0, 255, 255 } },
    { { -10,  10,  10 }, { 255,   0, 255, 255 } },
    { { -10, -10,  10 }, { 255,   0, 255, 255 } },
    { {  10, -10, -10 }, {   0, 255,   0, 255 } },
    { {  10,  10, -10 }, {   0, 255,   0, 255 } },
    { {  10,  10,  10 }, {   0, 255,   0, 255 } },
    { {  10, -10,  10 }, {   0, 255,   0, 255 } },
    { { -10, -10, -10 }, {   0,   0, 255, 255 } },
    { { -10, -10,  10 }, {   0,   0, 255, 255 } },
    { {  10, -10,  10 }, {   0,   0, 255, 255 } },
    { {  10, -10, -10 }, {   0,   0, 255, 255 } },
    { { -10,  10, -10 }, {   0, 255, 255, 255 } },
    { { -10,  10,  10 }, {   0, 255, 255, 255 } },
    { {  10,  10,  10 }, {   0, 255, 255, 255 } },
    { {  10,  10, -10 }, {   0, 255, 255, 255 } },
};

Uint16 cubeIndices[36] = {
     0,  1,  2,  0,  2,  3,
     6,  5,  4,  7,  6,  4,
     8,  9, 10,  8, 10, 11,
    14, 13, 12, 15, 14, 12,
    16, 17, 18, 16, 18, 19,
    22, 21, 20, 23, 22, 20,
};

} // namespace

Renderer::Renderer(GPUContext *gpu, ResourceManager *resources, Uploader &uploader, MsaaMode msaa)
    : gpu_(gpu)
    , gbuffer_(gpu->Device(), gpu->Size())
    , uploader_(uploader)
    , msaa_(msaa)
    , size_(gpu->Size()) {

    auto device = gpu_->Device();
    auto sampleCount = ChooseSampleCount(device, gpu_->SwapchainFormat(), msaa);

    if (msaa_ == MsaaMode::None) {
        depth_att_ = gbuffer_.AddAttachment({
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        });
    } else {
        color_att_ = gbuffer_.AddAttachment({
            .format = gpu_->SwapchainFormat(),
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
            .sample_count = sampleCount,
        });
        resolve_att_ = gbuffer_.AddAttachment({
            .format = gpu_->SwapchainFormat(),
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        });
        depth_att_ = gbuffer_.AddAttachment({
            .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .sample_count = sampleCount,
        });
    }

    SDL_GPUShader *vert = resources->LoadShader("Cube.vert", 0, 1);
    SDL_GPUShader *frag = resources->LoadShader("Cube.frag");

    SDL_GPUColorTargetDescription colorDesc = {
        .format = gpu_->SwapchainFormat(),
    };

    SDL_GPUVertexBufferDescription vbDesc = {
        .slot = 0,
        .pitch = sizeof(PositionColorVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };

    SDL_GPUVertexAttribute attrs[2] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0 },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, .offset = offsetof(PositionColorVertex, color) },
    };

    SDL_GPUVertexInputState vertexInput = {
        .vertex_buffer_descriptions = &vbDesc,
        .num_vertex_buffers = 1,
        .vertex_attributes = attrs,
        .num_vertex_attributes = 2,
    };

    SDL_GPUDepthStencilState depthState = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true,
    };

    SDL_GPURasterizerState rasterState = {
        .fill_mode = SDL_GPU_FILLMODE_FILL,
        .cull_mode = SDL_GPU_CULLMODE_BACK,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    };

    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {
        .color_target_descriptions = &colorDesc,
        .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .has_depth_stencil_target = true,
    };

    SDL_GPUMultisampleState multisampleState = {
        .sample_count = sampleCount,
    };

    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = vertexInput,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterState,
        .multisample_state = multisampleState,
        .depth_stencil_state = depthState,
        .target_info = targetInfo,
    };

    pipeline_ = chk(SDL_CreateGPUGraphicsPipeline(device, &info));

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    auto vertSpan = std::span(cubeVerts);
    auto idxSpan = std::span(cubeIndices);
    index_count_ = static_cast<int>(idxSpan.size());

    SDL_GPUBufferCreateInfo vbInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = static_cast<Uint32>(vertSpan.size_bytes()),
    };
    vertex_buffer_ = chk(SDL_CreateGPUBuffer(device, &vbInfo));

    SDL_GPUBufferCreateInfo ibInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = static_cast<Uint32>(idxSpan.size_bytes()),
    };
    index_buffer_ = chk(SDL_CreateGPUBuffer(device, &ibInfo));

    uploader_.Begin();
    uploader_.Buffer(vertex_buffer_, 0, std::as_bytes(vertSpan));
    uploader_.Buffer(index_buffer_, 0, std::as_bytes(idxSpan));
    uploader_.End();
}

Renderer::~Renderer() {
    auto device = gpu_->Device();
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_);
    SDL_ReleaseGPUBuffer(device, vertex_buffer_);
    SDL_ReleaseGPUBuffer(device, index_buffer_);
}

void Renderer::Resize(glm::ivec2 size) {
    size_ = size;
    gbuffer_.Resize(size);
}

void Renderer::Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, const glm::mat4 &viewProj, Scene &scene) {
    glm::mat4 mvp = viewProj * scene.ModelMatrix();
    SDL_PushGPUVertexUniformData(cmdbuf, 0, glm::value_ptr(mvp), sizeof(glm::mat4));

    SDL_GPUColorTargetInfo colorInfo = {
        .clear_color = SDL_FColor{ 0.1f, 0.1f, 0.2f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
    };

    if (msaa_ == MsaaMode::None) {
        colorInfo.texture = swapchain;
        colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    } else {
        colorInfo.texture = gbuffer_.GetTexture(color_att_);
        colorInfo.store_op = SDL_GPU_STOREOP_RESOLVE;
        colorInfo.resolve_texture = gbuffer_.GetTexture(resolve_att_);
    }

    SDL_GPUDepthStencilTargetInfo depthInfo = {
        .texture = gbuffer_.GetTexture(depth_att_),
        .clear_depth = 1.0f,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
        .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
    };

    SDL_GPURenderPass *pass = chk(SDL_BeginGPURenderPass(cmdbuf, &colorInfo, 1, &depthInfo));

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);

    SDL_GPUBufferBinding vbBind = { .buffer = vertex_buffer_ };
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

    SDL_GPUBufferBinding ibBind = { .buffer = index_buffer_ };
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(pass, index_count_, 1, 0, 0, 0);

    SDL_EndGPURenderPass(pass);

    if (msaa_ != MsaaMode::None) {
        SDL_GPUBlitInfo blitInfo = {
            .source = {
                .texture = gbuffer_.GetTexture(resolve_att_),
                .w = static_cast<Uint32>(size_.x),
                .h = static_cast<Uint32>(size_.y),
            },
            .destination = {
                .texture = swapchain,
                .w = static_cast<Uint32>(size_.x),
                .h = static_cast<Uint32>(size_.y),
            },
            .load_op = SDL_GPU_LOADOP_DONT_CARE,
            .filter = SDL_GPU_FILTER_LINEAR,
        };
        SDL_BlitGPUTexture(cmdbuf, &blitInfo);
    }
}
