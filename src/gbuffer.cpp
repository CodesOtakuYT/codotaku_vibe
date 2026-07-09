#include <gbuffer.hpp>

GBuffer::GBuffer(SDL_GPUDevice *device, glm::ivec2 size)
    : device_(device), size_(size) {}

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
        .width = static_cast<Uint32>(size_.x),
        .height = static_cast<Uint32>(size_.y),
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

void GBuffer::Resize(glm::ivec2 size) {
    if (size == size_) return;
    size_ = size;
    for (auto &att : attachments_) {
        DestroyAttachment(att);
        CreateAttachment(att);
    }
}
