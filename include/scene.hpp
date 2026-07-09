#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>
#include <span>
#include <vector>

struct PositionTextureVertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Geometry {
    std::vector<PositionTextureVertex> vertices;
    std::vector<Uint16> indices;
};

struct Instance {
    size_t geometry_index = 0;
    size_t material_index = 0;
    glm::mat4 transform{1.0f};
};

class Scene {
public:
    Scene();
    ~Scene() = default;

    void Update(float dt);
    std::span<const Geometry> Geometries() const { return geometries_; }
    std::span<const Instance> Instances() const { return instances_; }

private:
    std::vector<Geometry> geometries_;
    std::vector<Instance> instances_;
    float time_ = 0;
};
