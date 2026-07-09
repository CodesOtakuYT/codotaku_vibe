#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>
#include <span>
#include <vector>

struct PositionColorVertex {
    glm::vec3 pos;
    glm::u8vec4 color;
};

struct Geometry {
    std::vector<PositionColorVertex> vertices;
    std::vector<Uint16> indices;
};

struct Instance {
    glm::vec3 position{0.0f};
    float rotation = 0.0f;
    glm::vec3 rotation_axis{0.0f, 1.0f, 0.0f};

    glm::mat4 Transform() const;
};

class Scene {
public:
    Scene();
    ~Scene() = default;

    void Update(float dt);
    const Geometry &GetGeometry() const { return geometry_; }
    std::span<const Instance> Instances() const { return instances_; }

private:
    Geometry geometry_;
    std::vector<Instance> instances_;
};
