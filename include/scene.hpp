#pragma once

#include <glm/glm.hpp>

class Scene {
public:
    Scene() = default;

    void Update(float dt);
    glm::mat4 ModelMatrix() const;

private:
    float rotation_ = 0.0f;
};
