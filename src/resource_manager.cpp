#include <resource_manager.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>

namespace {

auto InferStage(const char *filename) -> SDL_GPUShaderStage {
    auto sv = std::string_view(filename);
    auto dot = sv.rfind('.');
    if (dot == std::string_view::npos)
        throw std::runtime_error(std::format("Cannot infer shader stage from: {}", filename));
    auto ext = sv.substr(dot);
    if (ext == ".vert") return SDL_GPU_SHADERSTAGE_VERTEX;
    if (ext == ".frag") return SDL_GPU_SHADERSTAGE_FRAGMENT;
    throw std::runtime_error(std::format("Unknown shader extension: {}", ext));
}

} // namespace

ResourceManager::ResourceManager(SDL_GPUDevice *device)
    : device_(device) {
    const char *p = chk(SDL_GetBasePath());
    base_path_ = p;

    auto formats = SDL_GetGPUShaderFormats(device_);
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        shader_dir_ = "SPIRV"; shader_ext_ = "spv";
        entrypoint_ = "main";
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        shader_dir_ = "DXIL"; shader_ext_ = "dxil";
        entrypoint_ = "main";
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        shader_dir_ = "MSL"; shader_ext_ = "msl";
        entrypoint_ = "main0";
    }
}

SDL_GPUShader *ResourceManager::LoadShader(const char *filename, Uint32 samplerCount, Uint32 uniformBufferCount) {
    auto stage = InferStage(filename);
    auto path = std::format("{}/assets/shaders/compiled/{}/{}.{}", base_path_, shader_dir_, filename, shader_ext_);

    auto it = bytecode_cache_.find(filename);
    if (it == bytecode_cache_.end()) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            throw std::runtime_error(std::format("Failed to open shader: {}", path));

        auto end = file.tellg();
        if (end < 0)
            throw std::runtime_error(std::format("Failed to read shader size: {}", path));

        auto size = static_cast<size_t>(end);
        file.seekg(0);

        std::vector<Uint8> code(size);
        file.read(reinterpret_cast<char *>(code.data()), static_cast<std::streamsize>(size));
        if (static_cast<size_t>(file.gcount()) != size)
            throw std::runtime_error(std::format("Short read on shader: {}", path));

        it = bytecode_cache_.emplace(filename, std::move(code)).first;
    }

    SDL_GPUShaderFormat format;
    if (shader_ext_ == "spv") format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if (shader_ext_ == "dxil") format = SDL_GPU_SHADERFORMAT_DXIL;
    else format = SDL_GPU_SHADERFORMAT_MSL;

    const auto &code = it->second;
    SDL_GPUShaderCreateInfo shaderInfo = {
        .code_size = code.size(),
        .code = code.data(),
        .entrypoint = entrypoint_,
        .format = format,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_uniform_buffers = uniformBufferCount,
    };
    return chk(SDL_CreateGPUShader(device_, &shaderInfo));
}
