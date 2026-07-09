#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>
#include <vector>

struct AttachmentDesc {
    SDL_GPUTextureFormat format;
    Uint32 usage;
    SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1;
};

class GBuffer {
public:
    GBuffer(SDL_GPUDevice *device, glm::ivec2 size);
    ~GBuffer();

    int AddAttachment(AttachmentDesc desc);

    SDL_GPUTexture *GetTexture(int index) const;

    void Resize(glm::ivec2 size);

    glm::ivec2 Size() const { return size_; }

private:
    struct Attachment {
        AttachmentDesc desc;
        SDL_GPUTexture *texture = nullptr;
    };

    void CreateAttachment(Attachment &att);
    void DestroyAttachment(Attachment &att);

    SDL_GPUDevice *device_ = nullptr;
    std::vector<Attachment> attachments_;
    glm::ivec2 size_{};
};
