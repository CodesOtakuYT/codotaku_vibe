#include <uploader.hpp>

#include <cstring>

Uploader::Uploader(SDL_GPUDevice *device)
    : device_(device) {}

void Uploader::Begin() {
    copies_.clear();
    textures_.clear();
}

void Uploader::Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, std::span<const std::byte> data) {
    copies_.push_back({dst, data.data(), dst_offset, static_cast<Uint32>(data.size())});
}

void Uploader::Texture(SDL_GPUTexture *tex, int w, int h, const void *pixels) {
    textures_.push_back({tex, pixels, w, h});
}

void Uploader::End() {
    Uint32 bufTotal = 0;
    for (auto &c : copies_) bufTotal += c.size;

    Uint32 texTotal = 0;
    for (auto &t : textures_) texTotal += t.w * t.h * 4;

    Uint32 total = bufTotal + texTotal;
    if (total == 0) return;

    SDL_GPUTransferBufferCreateInfo tbInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = total,
    };
    auto *transfer = chk(SDL_CreateGPUTransferBuffer(device_, &tbInfo));
    auto *mapped = static_cast<Uint8 *>(chk(SDL_MapGPUTransferBuffer(device_, transfer, false)));

    Uint32 offset = 0;
    for (auto &c : copies_) {
        std::memcpy(mapped + offset, c.data, c.size);
        offset += c.size;
    }
    for (auto &t : textures_) {
        Uint32 rowBytes = t.w * 4;
        for (int y = 0; y < t.h; ++y) {
            std::memcpy(mapped + offset + y * rowBytes,
                        static_cast<const Uint8 *>(t.data) + y * rowBytes,
                        rowBytes);
        }
        offset += t.h * rowBytes;
    }

    SDL_UnmapGPUTransferBuffer(device_, transfer);

    auto *cmdbuf = chk(SDL_AcquireGPUCommandBuffer(device_));
    auto *copyPass = chk(SDL_BeginGPUCopyPass(cmdbuf));

    offset = 0;
    for (const auto &c : copies_) {
        SDL_GPUTransferBufferLocation srcLoc = { .transfer_buffer = transfer, .offset = offset };
        SDL_GPUBufferRegion dstReg = { .buffer = c.buffer, .offset = c.dst_offset, .size = c.size };
        SDL_UploadToGPUBuffer(copyPass, &srcLoc, &dstReg, false);
        offset += c.size;
    }
    for (const auto &t : textures_) {
        Uint32 rowBytes = t.w * 4;
        Uint32 texBytes = t.h * rowBytes;
        SDL_GPUTextureTransferInfo srcInfo = {
            .transfer_buffer = transfer,
            .offset = offset,
            .pixels_per_row = static_cast<Uint32>(t.w),
            .rows_per_layer = static_cast<Uint32>(t.h),
        };
        SDL_GPUTextureRegion dstRegion = {
            .texture = t.texture,
            .w = static_cast<Uint32>(t.w),
            .h = static_cast<Uint32>(t.h),
            .d = 1,
        };
        SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);
        offset += texBytes;
    }

    SDL_EndGPUCopyPass(copyPass);
    chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
    SDL_ReleaseGPUTransferBuffer(device_, transfer);
    copies_.clear();
    textures_.clear();
}
