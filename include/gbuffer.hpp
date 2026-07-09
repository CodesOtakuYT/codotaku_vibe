#pragma once

#include <sdl.hpp>
#include <vector>

enum class AttachmentType : Uint8 { Color, DepthStencil };

struct AttachmentDesc {
    SDL_GPUTextureFormat format;
    AttachmentType type;
    SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;
};

class GBuffer {
public:
    GBuffer(SDL_GPUDevice *device, int width, int height);
    ~GBuffer();

    int AddAttachment(AttachmentDesc desc);

    SDL_GPUTexture *GetTexture(int index) const;
    SDL_GPUTexture *GetResolveTexture(int index) const;

    void Resize(int width, int height);

    int Width() const { return width_; }
    int Height() const { return height_; }

private:
    struct Attachment {
        AttachmentDesc desc;
        SDL_GPUTexture *texture = nullptr;
        SDL_GPUTexture *resolve_texture = nullptr;
    };

    void CreateAttachment(Attachment &att);
    void DestroyAttachment(Attachment &att);

    SDL_GPUDevice *device_ = nullptr;
    std::vector<Attachment> attachments_;
    int width_ = 0;
    int height_ = 0;
};
