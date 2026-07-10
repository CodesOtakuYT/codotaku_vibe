#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>
#include <span>
#include <vector>

#include <entt/entity/registry.hpp>
#include <components.hpp>

struct PositionTextureVertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Geometry {
    std::vector<PositionTextureVertex> vertices;
    std::vector<Uint32> indices;
};

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;

    size_t AddGeometry(std::span<const PositionTextureVertex> vertices,
                       std::span<const Uint32> indices);
    size_t AddGeometry(const ::Geometry &geometry);

    entt::entity CreateEntity(GeometryRef geoRef, MaterialRef matRef,
                              const glm::mat4 &transform = glm::mat4{1});

    entt::registry &Registry() { return registry_; }
    const entt::registry &Registry() const { return registry_; }

    std::span<const ::Geometry> Geometries() const { return geometries_; }
    ::Geometry &GetGeometry(size_t index) { return geometries_[index]; }
    const ::Geometry &GetGeometry(size_t index) const { return geometries_[index]; }

private:
    entt::registry registry_;
    std::vector<::Geometry> geometries_;
};
