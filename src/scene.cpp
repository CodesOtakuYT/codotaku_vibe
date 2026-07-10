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

auto Scene::CreateEntity(GeometryRef geoRef, MaterialRef matRef,
                         const glm::mat4 &transform) -> entt::entity {
    auto entity = registry_.create();
    registry_.emplace<GeometryRef>(entity, geoRef);
    registry_.emplace<MaterialRef>(entity, matRef);
    registry_.emplace<Transform>(entity, transform);
    return entity;
}
