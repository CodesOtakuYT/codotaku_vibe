#pragma once

#include <sdl.hpp>
#include <vector>

class Uploader {
public:
    explicit Uploader(SDL_GPUDevice *device);
    ~Uploader() = default;

    void Begin(Uint32 total_size);
    void Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, const void *data, Uint32 size);
    void End();

private:
    struct BufferCopy {
        SDL_GPUBuffer *buffer;
        Uint32 src_offset;
        Uint32 dst_offset;
        Uint32 size;
    };

    SDL_GPUDevice *device_;
    SDL_GPUTransferBuffer *transfer_ = nullptr;
    SDL_GPUCommandBuffer *cmdbuf_ = nullptr;
    Uint8 *mapped_ = nullptr;
    Uint32 current_offset_ = 0;
    Uint32 total_size_ = 0;
    std::vector<BufferCopy> copies_;
};
