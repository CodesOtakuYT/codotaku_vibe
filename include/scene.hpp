#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>
#include <span>
#include <vector>

struct PositionColorVertex {
    glm::vec3 pos;
    glm::u8vec4 color;
};

struct Mesh {
    std::vector<PositionColorVertex> vertices;
    std::vector<Uint16> indices;
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
    std::span<const Mesh> Meshes() const { return meshes_; }

private:
    std::vector<Mesh> meshes_;
};
