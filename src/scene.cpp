#include <scene.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace {

constexpr PositionColorVertex cubeVerts[24] = {
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

constexpr Uint16 cubeIndices[36] = {
     0,  1,  2,  0,  2,  3,
     6,  5,  4,  7,  6,  4,
     8,  9, 10,  8, 10, 11,
    14, 13, 12, 15, 14, 12,
    16, 17, 18, 16, 18, 19,
    22, 21, 20, 23, 22, 20,
};

} // namespace

Scene::Scene() {
    Mesh m0{};
    m0.vertices.assign(std::begin(cubeVerts), std::end(cubeVerts));
    m0.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));
    meshes_.push_back(std::move(m0));

    Mesh m1{};
    m1.vertices.assign(std::begin(cubeVerts), std::end(cubeVerts));
    m1.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));
    m1.position = { 15.0f, 0.0f, 0.0f };
    m1.rotation_axis = { 1.0f, 0.0f, 0.0f };
    meshes_.push_back(std::move(m1));

    Mesh m2{};
    m2.vertices.assign(std::begin(cubeVerts), std::end(cubeVerts));
    m2.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));
    m2.position = { -15.0f, 0.0f, -15.0f };
    m2.rotation_axis = { 0.0f, 0.0f, 1.0f };
    meshes_.push_back(std::move(m2));
}

void Scene::Update(float dt) {
    auto &m0 = meshes_[0];
    m0.rotation += dt * 0.5f;

    auto &m1 = meshes_[1];
    float t = m0.rotation;
    m1.position.x = 15.0f * cosf(t * 0.7f);
    m1.position.z = 15.0f * sinf(t * 0.7f);
    m1.rotation += dt * 0.8f;

    auto &m2 = meshes_[2];
    m2.position.x = -20.0f * cosf(t * 0.5f);
    m2.position.z = -20.0f * sinf(t * 0.5f);
    m2.position.y = 10.0f + 5.0f * sinf(t * 0.3f);
    m2.rotation += dt * 0.3f;
}

glm::mat4 Mesh::Transform() const {
    return glm::translate(glm::mat4(1.0f), position) *
           glm::rotate(glm::mat4(1.0f), rotation, rotation_axis);
}
