#pragma once

#include <sdl.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class ResourceManager {
public:
    explicit ResourceManager(SDL_GPUDevice *device);
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    SDL_GPUShader *LoadShader(const char *filename, Uint32 samplerCount = 0, Uint32 uniformBufferCount = 0);

private:
    SDL_GPUDevice *device_;
    std::string base_path_;
    std::string_view shader_dir_;
    std::string_view shader_ext_;
    const char *entrypoint_ = "main";
    std::unordered_map<std::string, std::vector<Uint8>> bytecode_cache_;
};
