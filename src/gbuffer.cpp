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

SDL_GPUTexture *GBuffer::GetResolveTexture(int index) const {
    return attachments_[index].resolve_texture;
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

void GBuffer::CreateAttachment(Attachment &att) {
    Uint32 usage = 0;
    if (att.desc.type == AttachmentType::Color) {
        usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        if (att.desc.sample_count == SDL_GPU_SAMPLECOUNT_1)
            usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
    } else {
        usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    }

    SDL_GPUTextureCreateInfo tInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = att.desc.format,
        .usage = usage,
        .width = static_cast<Uint32>(width_),
        .height = static_cast<Uint32>(height_),
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = att.desc.sample_count,
    };
    att.texture = chk(SDL_CreateGPUTexture(device_, &tInfo));

    if (att.desc.sample_count > SDL_GPU_SAMPLECOUNT_1 && att.desc.type == AttachmentType::Color) {
        SDL_GPUTextureCreateInfo rInfo = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = att.desc.format,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = static_cast<Uint32>(width_),
            .height = static_cast<Uint32>(height_),
            .layer_count_or_depth = 1,
            .num_levels = 1,
        };
        att.resolve_texture = chk(SDL_CreateGPUTexture(device_, &rInfo));
    }
}

void GBuffer::DestroyAttachment(Attachment &att) {
    if (att.resolve_texture) SDL_ReleaseGPUTexture(device_, att.resolve_texture);
    if (att.texture) SDL_ReleaseGPUTexture(device_, att.texture);
    att.texture = nullptr;
    att.resolve_texture = nullptr;
}
