#pragma once

#include <cstddef>
#include <string_view>

class Scene;

size_t LoadGLTF(std::string_view path, Scene &scene);
