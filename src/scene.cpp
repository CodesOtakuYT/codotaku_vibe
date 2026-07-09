#include <scene.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr PositionTextureVertex cubeVerts[24] = {
    { { -10, -10, -10 }, { 0, 0 } },
    { {  10, -10, -10 }, { 1, 0 } },
    { {  10,  10, -10 }, { 1, 1 } },
    { { -10,  10, -10 }, { 0, 1 } },
    { { -10, -10,  10 }, { 0, 0 } },
    { {  10, -10,  10 }, { 1, 0 } },
    { {  10,  10,  10 }, { 1, 1 } },
    { { -10,  10,  10 }, { 0, 1 } },
    { { -10, -10, -10 }, { 0, 0 } },
    { { -10,  10, -10 }, { 1, 0 } },
    { { -10,  10,  10 }, { 1, 1 } },
    { { -10, -10,  10 }, { 0, 1 } },
    { {  10, -10, -10 }, { 0, 0 } },
    { {  10,  10, -10 }, { 1, 0 } },
    { {  10,  10,  10 }, { 1, 1 } },
    { {  10, -10,  10 }, { 0, 1 } },
    { { -10, -10, -10 }, { 0, 0 } },
    { { -10, -10,  10 }, { 1, 0 } },
    { {  10, -10,  10 }, { 1, 1 } },
    { {  10, -10, -10 }, { 0, 1 } },
    { { -10,  10, -10 }, { 0, 0 } },
    { { -10,  10,  10 }, { 1, 0 } },
    { {  10,  10,  10 }, { 1, 1 } },
    { {  10,  10, -10 }, { 0, 1 } },
};

constexpr Uint16 cubeIndices[36] = {
     0,  1,  2,  0,  2,  3,
     6,  5,  4,  7,  6,  4,
     8,  9, 10,  8, 10, 11,
    14, 13, 12, 15, 14, 12,
    16, 17, 18, 16, 18, 19,
    22, 21, 20, 23, 22, 20,
};

constexpr PositionTextureVertex pyramidVerts[5] = {
    { {   0, 10,   0 }, { 0.5f, 1.0f } },
    { { -10, -10, -10 }, { 0.0f, 0.0f } },
    { {  10, -10, -10 }, { 1.0f, 0.0f } },
    { {  10, -10,  10 }, { 1.0f, 1.0f } },
    { { -10, -10,  10 }, { 0.0f, 1.0f } },
};

constexpr Uint16 pyramidIndices[18] = {
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 1,
    1, 3, 2,
    1, 4, 3,
};

} // namespace

Scene::Scene() {
    auto &cube = geometries_.emplace_back();
    cube.vertices.assign(std::begin(cubeVerts), std::end(cubeVerts));
    cube.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));

    auto &pyramid = geometries_.emplace_back();
    pyramid.vertices.assign(std::begin(pyramidVerts), std::end(pyramidVerts));
    pyramid.indices.assign(std::begin(pyramidIndices), std::end(pyramidIndices));

    instances_.push_back({ .geometry_index = 0, .material_index = 0 });
    instances_.push_back({ .geometry_index = 0, .material_index = 0 });
    instances_.push_back({ .geometry_index = 1, .material_index = 0 });
}

void Scene::Update(float dt) {
    time_ += dt;

    auto &i0 = instances_[0];
    float r0 = time_ * 0.5f;
    i0.transform = glm::rotate(glm::mat4{1}, r0, {0, 1, 0});

    auto &i1 = instances_[1];
    glm::vec3 p1{15.0f * cosf(r0 * 0.7f), 0.0f, 15.0f * sinf(r0 * 0.7f)};
    i1.transform = glm::translate(glm::mat4{1}, p1) *
                   glm::rotate(glm::mat4{1}, time_ * 0.8f, {1, 0, 0});

    auto &i2 = instances_[2];
    glm::vec3 p2{-20.0f * cosf(r0 * 0.5f), 10.0f + 5.0f * sinf(r0 * 0.3f), -20.0f * sinf(r0 * 0.5f)};
    i2.transform = glm::translate(glm::mat4{1}, p2) *
                   glm::rotate(glm::mat4{1}, time_ * 0.3f, {0, 0, 1});
}
