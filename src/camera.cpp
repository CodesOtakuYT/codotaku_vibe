#include <camera.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

OrbitCamera::OrbitCamera() = default;

void OrbitCamera::Event(const SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                mouse_down_ = true;
                last_mouse_ = glm::vec2(static_cast<float>(event.button.x), static_cast<float>(event.button.y));
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
                mouse_down_ = false;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (mouse_down_) {
                float dx = static_cast<float>(event.motion.x) - last_mouse_.x;
                float dy = static_cast<float>(event.motion.y) - last_mouse_.y;
                last_mouse_ = glm::vec2(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
                yaw_ += dx * 0.005f;
                pitch_ -= dy * 0.005f;
                pitch_ = glm::clamp(pitch_, -1.5f, 1.5f);
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            distance_ *= (event.wheel.y > 0) ? 0.9f : 1.1f;
            distance_ = glm::clamp(distance_, 5.0f, 200.0f);
            break;
    }
}

glm::mat4 OrbitCamera::ViewProjMatrix(glm::ivec2 viewport) const {
    glm::mat4 proj = glm::perspectiveFov(glm::radians(75.0f), static_cast<float>(viewport.x), static_cast<float>(viewport.y), 0.01f, 400.0f);
    proj[1][1] *= -1.0f;

    glm::vec3 eye(
        SDL_cosf(yaw_) * SDL_cosf(pitch_) * distance_,
        SDL_sinf(pitch_) * distance_,
        SDL_sinf(yaw_) * SDL_cosf(pitch_) * distance_
    );
    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return proj * view;
}
