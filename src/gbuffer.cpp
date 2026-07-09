#include <gbuffer.hpp>

GBuffer::GBuffer(SDL_GPUDevice *device, int width, int height)
    : device_(device), width_(width), height_(height) {}

GBuffer::~GBuffer() {
    for (auto &att : attachments_)
        DestroyAttachment(att);
}

int GBuffer::AddAttachment(AttachmentDesc desc) {
    int index = static_cast<int>(attachments_.size());
    auto &att = attachments_.emplace_back();
    att.desc = desc;
    CreateAttachment(att);
    return index;
}

SDL_GPUTexture *GBuffer::GetTexture(int index) const {
    return attachments_[index].texture;
}

void GBuffer::CreateAttachment(Attachment &att) {
    SDL_GPUTextureCreateInfo tInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = att.desc.format,
        .usage = att.desc.usage,
        .width = static_cast<Uint32>(width_),
        .height = static_cast<Uint32>(height_),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = att.desc.sample_count,
    };
    att.texture = chk(SDL_CreateGPUTexture(device_, &tInfo));
}

void GBuffer::DestroyAttachment(Attachment &att) {
    if (att.texture) SDL_ReleaseGPUTexture(device_, att.texture);
    att.texture = nullptr;
}

void GBuffer::Resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    for (auto &att : attachments_) {
        DestroyAttachment(att);
        CreateAttachment(att);
    }
}
