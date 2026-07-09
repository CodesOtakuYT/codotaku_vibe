#include <uploader.hpp>

#include <cstring>

Uploader::Uploader(SDL_GPUDevice *device)
    : device_(device) {}

void Uploader::Begin(Uint32 total_size) {
    total_size_ = total_size;
    current_offset_ = 0;
    copies_.clear();

    SDL_GPUTransferBufferCreateInfo tbInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = total_size_,
    };
    transfer_ = chk(SDL_CreateGPUTransferBuffer(device_, &tbInfo));
    mapped_ = static_cast<Uint8 *>(chk(SDL_MapGPUTransferBuffer(device_, transfer_, false)));
    cmdbuf_ = chk(SDL_AcquireGPUCommandBuffer(device_));
}

void Uploader::Buffer(SDL_GPUBuffer *dst, Uint32 dst_offset, const void *data, Uint32 size) {
    std::memcpy(mapped_ + current_offset_, data, size);
    copies_.push_back({dst, current_offset_, dst_offset, size});
    current_offset_ += size;
}

void Uploader::End() {
    SDL_UnmapGPUTransferBuffer(device_, transfer_);
    mapped_ = nullptr;

    SDL_GPUCopyPass *copy = chk(SDL_BeginGPUCopyPass(cmdbuf_));

    for (const auto &op : copies_) {
        SDL_GPUTransferBufferLocation srcLoc = { .transfer_buffer = transfer_, .offset = op.src_offset };
        SDL_GPUBufferRegion dstReg = { .buffer = op.buffer, .offset = op.dst_offset, .size = op.size };
        SDL_UploadToGPUBuffer(copy, &srcLoc, &dstReg, false);
    }

    SDL_EndGPUCopyPass(copy);
    chk(SDL_SubmitGPUCommandBuffer(cmdbuf_));
    SDL_ReleaseGPUTransferBuffer(device_, transfer_);
    transfer_ = nullptr;
    cmdbuf_ = nullptr;
}
