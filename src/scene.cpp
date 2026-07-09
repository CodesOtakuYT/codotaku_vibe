#include <scene.hpp>

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
