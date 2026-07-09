#include <scene.hpp>

#include <glm/gtc/matrix_transform.hpp>

void Scene::Update(float dt) {
    rotation_ += dt * 0.5f;
}

glm::mat4 Scene::ModelMatrix() const {
    return glm::rotate(glm::mat4(1.0f), rotation_, glm::vec3(0.0f, 1.0f, 0.0f));
}
