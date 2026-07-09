#pragma once

#include <sdl.hpp>
#include <vector>

class Uploader {
public:
    explicit Uploader(SDL_GPUDevice *device);
    ~Uploader() = default;

    void Begin();
    void Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, const void *data, Uint32 size);
    void End();

private:
    struct BufferCopy {
        SDL_GPUBuffer *buffer;
        const void *data;
        Uint32 dst_offset;
        Uint32 size;
    };

    SDL_GPUDevice *device_;
    std::vector<BufferCopy> copies_;
};
