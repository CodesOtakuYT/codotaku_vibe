#pragma once

#include <glm/glm.hpp>
#include <cstddef>

struct Transform {
    glm::mat4 value{1.0f};
};

struct GeometryRef {
    std::size_t index = 0;
};

struct MaterialRef {
    std::size_t index = 0;
};
