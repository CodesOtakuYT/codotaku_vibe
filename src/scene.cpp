#include <scene.hpp>
#include <uploader.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <span>

Scene::Scene(SDL_GPUDevice *device, Uploader &uploader)
    : device_(device) {

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
    index_count_ = static_cast<int>(idxSpan.size());

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

    uploader.Begin(static_cast<Uint32>(vertSpan.size_bytes() + idxSpan.size_bytes()));
    uploader.Buffer(vertex_buffer_, 0, vertSpan.data(), static_cast<Uint32>(vertSpan.size_bytes()));
    uploader.Buffer(index_buffer_, 0, idxSpan.data(), static_cast<Uint32>(idxSpan.size_bytes()));
    uploader.End();
}

Scene::~Scene() {
    SDL_ReleaseGPUBuffer(device_, vertex_buffer_);
    SDL_ReleaseGPUBuffer(device_, index_buffer_);
}

void Scene::Update(float dt) {
    rotation_ += dt * 0.5f;
}

void Scene::Render(SDL_GPURenderPass *pass, SDL_GPUCommandBuffer *cmdbuf, const glm::mat4 &viewProj) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), rotation_, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 mvp = viewProj * model;

    SDL_PushGPUVertexUniformData(cmdbuf, 0, glm::value_ptr(mvp), sizeof(glm::mat4));

    SDL_GPUBufferBinding vbBind = { .buffer = vertex_buffer_ };
    SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);

    SDL_GPUBufferBinding ibBind = { .buffer = index_buffer_ };
    SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(pass, index_count_, 1, 0, 0, 0);
}
