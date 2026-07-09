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
    geometry_.vertices.assign(std::begin(cubeVerts), std::end(cubeVerts));
    geometry_.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));

    instances_.push_back({});

    instances_.push_back({
        .position = { 15.0f, 0.0f, 0.0f },
        .rotation_axis = { 1.0f, 0.0f, 0.0f },
    });

    instances_.push_back({
        .position = { -15.0f, 0.0f, -15.0f },
        .rotation_axis = { 0.0f, 0.0f, 1.0f },
    });
}

void Scene::Update(float dt) {
    auto &i0 = instances_[0];
    i0.rotation += dt * 0.5f;

    auto &i1 = instances_[1];
    float t = i0.rotation;
    i1.position.x = 15.0f * cosf(t * 0.7f);
    i1.position.z = 15.0f * sinf(t * 0.7f);
    i1.rotation += dt * 0.8f;

    auto &i2 = instances_[2];
    i2.position.x = -20.0f * cosf(t * 0.5f);
    i2.position.z = -20.0f * sinf(t * 0.5f);
    i2.position.y = 10.0f + 5.0f * sinf(t * 0.3f);
    i2.rotation += dt * 0.3f;
}

glm::mat4 Instance::Transform() const {
    return glm::translate(glm::mat4(1.0f), position) *
           glm::rotate(glm::mat4(1.0f), rotation, rotation_axis);
}
