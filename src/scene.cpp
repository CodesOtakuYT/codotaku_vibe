#include <scene.hpp>

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

auto Scene::CreateCube() -> Geometry {
    Geometry g;
    g.vertices.assign(std::begin(cubeVerts), std::end(cubeVerts));
    g.indices.assign(std::begin(cubeIndices), std::end(cubeIndices));
    return g;
}

auto Scene::CreatePyramid() -> Geometry {
    Geometry g;
    g.vertices.assign(std::begin(pyramidVerts), std::end(pyramidVerts));
    g.indices.assign(std::begin(pyramidIndices), std::end(pyramidIndices));
    return g;
}

auto Scene::AddGeometry(std::span<const PositionTextureVertex> vertices,
                        std::span<const Uint16> indices) -> size_t {
    auto &g = geometries_.emplace_back();
    g.vertices.assign(vertices.begin(), vertices.end());
    g.indices.assign(indices.begin(), indices.end());
    return geometries_.size() - 1;
}

auto Scene::AddGeometry(const ::Geometry &geometry) -> size_t {
    auto &g = geometries_.emplace_back();
    g.vertices.assign(geometry.vertices.begin(), geometry.vertices.end());
    g.indices.assign(geometry.indices.begin(), geometry.indices.end());
    return geometries_.size() - 1;
}

auto Scene::AddInstance(size_t geometry_index, size_t material_index,
                        const glm::mat4 &transform) -> size_t {
    instances_.push_back({ geometry_index, material_index, transform });
    return instances_.size() - 1;
}
