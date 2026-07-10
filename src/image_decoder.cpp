#include <image_decoder.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <SDL3/SDL_surface.h>

SDL_Surface *DecodeImageBytes(const std::byte *data, size_t length) {
    int w, h, channels;
    unsigned char *pixels = stbi_load_from_memory(
        reinterpret_cast<const unsigned char *>(data),
        static_cast<int>(length),
        &w, &h, &channels, STBI_rgb_alpha
    );
    if (!pixels)
        return nullptr;

    SDL_Surface *temp = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    if (!temp) {
        stbi_image_free(pixels);
        return nullptr;
    }

    SDL_Surface *surface = SDL_ConvertSurface(temp, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(temp);
    stbi_image_free(pixels);

    return surface;
}
