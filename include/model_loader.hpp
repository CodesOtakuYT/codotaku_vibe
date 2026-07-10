#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include <SDL3/SDL_surface.h>

class Scene;

struct GltfLoadResult {
    size_t entityCount = 0;
    std::vector<SDL_Surface *> materialSurfaces;
};

GltfLoadResult LoadGLTF(std::string_view path, Scene &scene, size_t materialBaseOffset = 0);
