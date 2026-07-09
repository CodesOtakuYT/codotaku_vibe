#pragma once

#include <sdl.hpp>
#include <cstddef>
#include <span>
#include <vector>

class Uploader {
public:
    explicit Uploader(SDL_GPUDevice *device);
    ~Uploader() = default;

    void Begin();
    void Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, std::span<const std::byte> data);
    void End();

private:
    struct BufferCopy {
        SDL_GPUBuffer *buffer;
        const std::byte *data;
        Uint32 dst_offset;
        Uint32 size;
    };

    SDL_GPUDevice *device_;
    std::vector<BufferCopy> copies_;
};
