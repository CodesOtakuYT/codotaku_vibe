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
    Scene() = default;
    ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = default;
    Scene& operator=(Scene&&) = default;

    static Geometry CreateCube();
    static Geometry CreatePyramid();

    size_t AddGeometry(std::span<const PositionTextureVertex> vertices,
                       std::span<const Uint16> indices);
    size_t AddGeometry(const ::Geometry &geometry);

    size_t AddInstance(size_t geometry_index, size_t material_index,
                       const glm::mat4 &transform = glm::mat4{1});

    std::span<const ::Geometry> Geometries() const { return geometries_; }
    std::span<const ::Instance> Instances() const { return instances_; }
    ::Instance &GetInstance(size_t index) { return instances_[index]; }

private:
    std::vector<::Geometry> geometries_;
    std::vector<::Instance> instances_;
};
