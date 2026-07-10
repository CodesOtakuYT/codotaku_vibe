#pragma once

#include <sdl.hpp>
#include <glm/glm.hpp>
#include <SDL3/SDL_surface.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Uploader;

struct TextureHandle {
    SDL_GPUTexture *texture = nullptr;
    SDL_GPUSampler *sampler = nullptr;
    glm::ivec2 size{};
};

class ResourceManager {
public:
    explicit ResourceManager(SDL_GPUDevice *device, Uploader &uploader);
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    SDL_GPUShader *LoadShader(const char *filename, Uint32 samplerCount = 0, Uint32 uniformBufferCount = 0);

    TextureHandle LoadTexture(const char *filename,
                              SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);

    TextureHandle LoadTextureFromSurface(SDL_Surface *surface,
                                         SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);

private:
    struct CachedTexture {
        TextureHandle handle;
        std::vector<Uint8> cpu_pixels;
        int w, h;
    };

    SDL_GPUDevice *device_;
    Uploader &uploader_;
    std::string base_path_;
    std::string_view shader_dir_;
    std::string_view shader_ext_;
    const char *entrypoint_ = "main";
    std::unordered_map<std::string, std::vector<Uint8>> bytecode_cache_;
    std::unordered_map<std::string, CachedTexture> texture_cache_;
    SDL_GPUSampler *default_sampler_ = nullptr;
};
