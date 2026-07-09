#include <renderer.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <vector>

Renderer::Renderer(SDL_Window *window, SDL_GPUDevice *device)
    : window_(window), device_(device) {
    base_path_ = chk(SDL_GetBasePath());
    SDL_GetWindowSizeInPixels(window_, &win_w_, &win_h_);

    auto formats = SDL_GetGPUShaderFormats(device_);
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        shader_format_ = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint_ = "main";
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        shader_format_ = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint_ = "main";
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        shader_format_ = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint_ = "main0";
    }

    SDL_GPUShader *vert = LoadShader("Cube.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    SDL_GPUShader *frag = LoadShader("Cube.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 0, 0);

    SDL_GPUColorTargetDescription colorDesc = {
        .format = SDL_GetGPUSwapchainTextureFormat(device_, window_),
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

    pipeline_ = chk(SDL_CreateGPUGraphicsPipeline(device_, &info));

    SDL_ReleaseGPUShader(device_, vert);
    SDL_ReleaseGPUShader(device_, frag);

    CreateCubeResources();
}

Renderer::~Renderer() {
    SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
    SDL_ReleaseGPUBuffer(device_, vertex_buffer_);
    SDL_ReleaseGPUBuffer(device_, index_buffer_);
    SDL_ReleaseGPUTexture(device_, depth_texture_);
}

auto Renderer::LoadShader(const char *filename, SDL_GPUShaderStage stage, Uint32 samplerCount, Uint32 uniformBufferCount) -> SDL_GPUShader * {
    const char *ext = (shader_format_ == SDL_GPU_SHADERFORMAT_SPIRV) ? "spv" :
                      (shader_format_ == SDL_GPU_SHADERFORMAT_DXIL) ? "dxil" :
                                                                      "msl";
    const char *dir = (shader_format_ == SDL_GPU_SHADERFORMAT_SPIRV) ? "SPIRV" :
                      (shader_format_ == SDL_GPU_SHADERFORMAT_DXIL) ? "DXIL" :
                                                                      "MSL";

    auto path = std::format("{}/assets/shaders/compiled/{}/{}.{}", base_path_, dir, filename, ext);

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    chk(file.is_open());

    auto size = static_cast<size_t>(file.tellg());
    file.seekg(0);

    std::vector<Uint8> code(size);
    file.read(reinterpret_cast<char *>(code.data()), static_cast<std::streamsize>(size));

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = code.size(),
        .code = code.data(),
        .entrypoint = entrypoint_,
        .format = shader_format_,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_uniform_buffers = uniformBufferCount,
    };

    return chk(SDL_CreateGPUShader(device_, &shaderInfo));
}

void Renderer::CreateCubeResources() {
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
    vertex_buffer_ = chk(SDL_CreateGPUBuffer(device_, &vbInfo));

    SDL_GPUBufferCreateInfo ibInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = static_cast<Uint32>(idxSpan.size_bytes()),
    };
    index_buffer_ = chk(SDL_CreateGPUBuffer(device_, &ibInfo));

    SDL_GPUTransferBufferCreateInfo tbInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = static_cast<Uint32>(vertSpan.size_bytes() + idxSpan.size_bytes()),
    };
    SDL_GPUTransferBuffer *transfer = chk(SDL_CreateGPUTransferBuffer(device_, &tbInfo));

    Uint8 *data = static_cast<Uint8 *>(chk(SDL_MapGPUTransferBuffer(device_, transfer, false)));
    std::memcpy(data, vertSpan.data(), vertSpan.size_bytes());
    std::memcpy(data + vertSpan.size_bytes(), idxSpan.data(), idxSpan.size_bytes());
    SDL_UnmapGPUTransferBuffer(device_, transfer);

    SDL_GPUCommandBuffer *cmdbuf = chk(SDL_AcquireGPUCommandBuffer(device_));
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
    SDL_ReleaseGPUTransferBuffer(device_, transfer);

    CreateDepthTexture();
}

void Renderer::CreateDepthTexture() {
    SDL_ReleaseGPUTexture(device_, depth_texture_);

    SDL_GPUTextureCreateInfo dtInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = static_cast<Uint32>(win_w_),
        .height = static_cast<Uint32>(win_h_),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    depth_texture_ = chk(SDL_CreateGPUTexture(device_, &dtInfo));
}

void Renderer::Render(float dt) {
    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(device_);
    if (!cmdbuf) return;

    SDL_GPUTexture *swapchainTexture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, window_, &swapchainTexture, nullptr, nullptr)) {
        chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
        return;
    }

    if (!swapchainTexture) {
        chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
        return;
    }

    int w, h;
    SDL_GetWindowSizeInPixels(window_, &w, &h);
    if (w != win_w_ || h != win_h_) {
        win_w_ = w;
        win_h_ = h;
        CreateDepthTexture();
    }

    glm::mat4 proj = glm::perspectiveFov(glm::radians(75.0f), static_cast<float>(win_w_), static_cast<float>(win_h_), 0.01f, 100.0f);
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
        .texture = swapchainTexture,
        .clear_color = SDL_FColor{ 0.1f, 0.1f, 0.2f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPUDepthStencilTargetInfo depthInfo = {
        .texture = depth_texture_,
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
    chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
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
