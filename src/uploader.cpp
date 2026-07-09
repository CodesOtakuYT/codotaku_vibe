#include <uploader.hpp>

#include <cstring>

Uploader::Uploader(SDL_GPUDevice *device)
    : device_(device) {}

void Uploader::Begin() {
    copies_.clear();
}

void Uploader::Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, const void *data, Uint32 size) {
    copies_.push_back({dst, data, dst_offset, size});
}

void Uploader::End() {
    Uint32 total = 0;
    for (auto &c : copies_) total += c.size;

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

    SDL_EndGPUCopyPass(copyPass);
    chk(SDL_SubmitGPUCommandBuffer(cmdbuf));
    SDL_ReleaseGPUTransferBuffer(device_, transfer);
    copies_.clear();
}
