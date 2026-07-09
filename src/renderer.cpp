#include <renderer.hpp>
#include <gpu_context.hpp>
#include <gbuffer.hpp>
#include <resource_manager.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <cstring>
#include <span>

Renderer::Renderer(GPUContext *gpu, GBuffer *gbuffer, ResourceManager *resources)
    : gpu_(gpu), gbuffer_(gbuffer), resources_(resources) {

    auto device = gpu_->Device();

    SDL_GPUShader *vert = resources_->LoadShader("Cube.vert", 0, 1);
    SDL_GPUShader *frag = resources_->LoadShader("Cube.frag");

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
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, .offset = offsetof(PositionColorVertex, r) },
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

    SDL_GPUGraphicsPipelineCreateInfo info = {
        .vertex_shader = vert,
        .fragment_shader = frag,
        .vertex_input_state = vertexInput,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = rasterState,
        .depth_stencil_state = depthState,
        .target_info = targetInfo,
    };

    pipeline_ = chk(SDL_CreateGPUGraphicsPipeline(device, &info));

    SDL_ReleaseGPUShader(device, vert);
    SDL_ReleaseGPUShader(device, frag);

    CreateCubeResources();
}

Renderer::~Renderer() {
    auto device = gpu_->Device();
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline_);
    SDL_ReleaseGPUBuffer(device, vertex_buffer_);
    SDL_ReleaseGPUBuffer(device, index_buffer_);
}

void Renderer::CreateCubeResources() {
    auto device = gpu_->Device();

    PositionColorVertex verts[24] = {
        { { -10, -10, -10 }, 255,   0,   0, 255 },
        { {  10, -10, -10 }, 255,   0,   0, 255 },
        { {  10,  10, -10 }, 255,   0,   0, 255 },
        { { -10,  10, -10 }, 255,   0,   0, 255 },
        { { -10, -10,  10 }, 255, 255,   0, 255 },
        { {  10, -10,  10 }, 255, 255,   0, 255 },
        { {  10,  10,  10 }, 255, 255,   0, 255 },
        { { -10,  10,  10 }, 255, 255,   0, 255 },
        { { -10, -10, -10 }, 255,   0, 255, 255 },
        { { -10,  10, -10 }, 255,   0, 255, 255 },
        { { -10,  10,  10 }, 255,   0, 255, 255 },
        { { -10, -10,  10 }, 255,   0, 255, 255 },
        { {  10, -10, -10 },   0, 255,   0, 255 },
        { {  10,  10, -10 },   0, 255,   0, 255 },
        { {  10,  10,  10 },   0, 255,   0, 255 },
        { {  10, -10,  10 },   0, 255,   0, 255 },
        { { -10, -10, -10 },   0,   0, 255, 255 },
        { { -10, -10,  10 },   0,   0, 255, 255 },
        { {  10, -10,  10 },   0,   0, 255, 255 },
        { {  10, -10, -10 },   0,   0, 255, 255 },
        { { -10,  10, -10 },   0, 255, 255, 255 },
        { { -10,  10,  10 },   0, 255, 255, 255 },
        { {  10,  10,  10 },   0, 255, 255, 255 },
        { {  10,  10, -10 },   0, 255, 255, 255 },
    };

    Uint16 indices[36] = {
         0,  1,  2,  0,  2,  3,
         6,  5,  4,  7,  6,  4,
         8,  9, 10,  8, 10, 11,
        14, 13, 12, 15, 14, 12,
        16, 17, 18, 16, 18, 19,
        22, 21, 20, 23, 22, 20,
    };

    auto vertSpan = std::span(verts);
    auto idxSpan = std::span(indices);

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

    SDL_GPUTransferBufferCreateInfo tbInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(vertSpan.size_bytes() + idxSpan.size_bytes()),
    };
    SDL_GPUTransferBuffer *transfer = chk(SDL_CreateGPUTransferBuffer(device, &tbInfo));

    Uint8 *data = static_cast<Uint8 *>(chk(SDL_MapGPUTransferBuffer(device, transfer, false)));
    std::memcpy(data, vertSpan.data(), vertSpan.size_bytes());
    std::memcpy(data + vertSpan.size_bytes(), idxSpan.data(), idxSpan.size_bytes());
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer *cmdbuf = chk(SDL_AcquireGPUCommandBuffer(device));
    SDL_GPUCopyPass *copy = chk(SDL_BeginGPUCopyPass(cmdbuf));

    SDL_GPUTransferBufferLocation srcLoc = { .transfer_buffer = transfer, .offset = 0 };

    SDL_GPUBufferRegion dstReg = { .buffer = vertex_buffer_, .size = static_cast<Uint32>(vertSpan.size_bytes()) };
    SDL_UploadToGPUBuffer(copy, &srcLoc, &dstReg, false);

    srcLoc.offset = static_cast<Uint32>(vertSpan.size_bytes());
    dstReg.buffer = index_buffer_;
    dstReg.size = static_cast<Uint32>(idxSpan.size_bytes());
    SDL_UploadToGPUBuffer(copy, &srcLoc, &dstReg, false);

    SDL_EndGPUCopyPass(copy);
    chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
    SDL_ReleaseGPUTransferBuffer(device, transfer);
}

void Renderer::Render(SDL_GPUCommandBuffer *cmdbuf, SDL_GPUTexture *swapchain, int w, int h, float dt) {
    glm::mat4 proj = glm::perspectiveFov(glm::radians(75.0f), static_cast<float>(w), static_cast<float>(h), 0.01f, 100.0f);
    proj[1][1] *= -1.0f;

    glm::vec3 eye(
        SDL_cosf(yaw_) * SDL_cosf(pitch_) * distance_,
        SDL_sinf(pitch_) * distance_,
        SDL_sinf(yaw_) * SDL_cosf(pitch_) * distance_
    );
    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), dt * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 mvp = proj * view * model;

    SDL_GPUColorTargetInfo colorInfo = {
        .texture = swapchain,
        .clear_color = SDL_FColor{ 0.1f, 0.1f, 0.2f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPUDepthStencilTargetInfo depthInfo = {
        .texture = gbuffer_->DepthTexture(),
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

    SDL_PushGPUVertexUniformData(cmdbuf, 0, glm::value_ptr(mvp), sizeof(glm::mat4));
    SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);

    SDL_EndGPURenderPass(pass);
}

void Renderer::Event(const SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                mouse_down_ = true;
                last_mouse_ = glm::vec2(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
                mouse_down_ = false;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (mouse_down_) {
                float dx = static_cast<float>(event.motion.x) - last_mouse_.x;
                float dy = static_cast<float>(event.motion.y) - last_mouse_.y;
                last_mouse_ = glm::vec2(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
                yaw_ += dx * 0.005f;
                pitch_ -= dy * 0.005f;
                pitch_ = glm::clamp(pitch_, -1.5f, 1.5f);
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            distance_ *= (event.wheel.y > 0) ? 0.9f : 1.1f;
            distance_ = glm::clamp(distance_, 5.0f, 80.0f);
            break;
    }
}
