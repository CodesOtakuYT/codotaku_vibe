#pragma once

#include <SDL3/SDL_surface.h>

#include <cstddef>

SDL_Surface *DecodeImageBytes(const std::byte *data, size_t length);
