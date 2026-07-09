#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL.h>

class OrbitCamera {
public:
    OrbitCamera();

    void Event(const SDL_Event &event);

    glm::mat4 ViewProjMatrix(int w, int h) const;

private:
    glm::vec2 last_mouse_{};
    bool mouse_down_ = false;
    float pitch_ = 0.3f;
    float yaw_ = 0.0f;
    float distance_ = 30.0f;
};
