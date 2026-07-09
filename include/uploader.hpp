#pragma once

#include <sdl.hpp>
#include <cstddef>
#include <span>
#include <vector>

class Uploader {
public:
    explicit Uploader(SDL_GPUDevice *device);
    ~Uploader() = default;

    Uploader(const Uploader&) = delete;
    Uploader& operator=(const Uploader&) = delete;
    Uploader(Uploader&&) = delete;
    Uploader& operator=(Uploader&&) = delete;

    void Begin();
    void Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, std::span<const std::byte> data);
    void Texture(SDL_GPUTexture *tex, int w, int h, const void *pixels);
    void End();

private:
    struct BufferCopy {
        SDL_GPUBuffer *buffer;
        const std::byte *data;
        Uint32 dst_offset;
        Uint32 size;
    };

    struct TextureCopy {
        SDL_GPUTexture *texture;
        const void *data;
        int w, h;
    };

    SDL_GPUDevice *device_;
    std::vector<BufferCopy> copies_;
    std::vector<TextureCopy> textures_;
};
