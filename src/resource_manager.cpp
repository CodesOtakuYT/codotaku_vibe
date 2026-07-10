#include <resource_manager.hpp>
#include <uploader.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <cstring>

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

ResourceManager::ResourceManager(SDL_GPUDevice *device, Uploader &uploader)
    : device_(device)
    , uploader_(uploader) {
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

ResourceManager::~ResourceManager() {
    for (auto &[key, ct] : texture_cache_) {
        if (ct.handle.texture) SDL_ReleaseGPUTexture(device_, ct.handle.texture);
    }
    if (default_sampler_) SDL_ReleaseGPUSampler(device_, default_sampler_);
}

SDL_GPUShader *ResourceManager::LoadShader(const char *filename, Uint32 samplerCount, Uint32 uniformBufferCount) {
    auto stage = InferStage(filename);
    auto path = std::format("{}assets/shaders/compiled/{}/{}.{}", base_path_, shader_dir_, filename, shader_ext_);

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

auto ResourceManager::LoadTexture(const char *filename, SDL_GPUTextureFormat format) -> TextureHandle {
    auto it = texture_cache_.find(filename);
    if (it != texture_cache_.end()) {
        it->second.handle.sampler = default_sampler_;
        return it->second.handle;
    }

    auto path = std::format("{}/assets/textures/{}", base_path_, filename);
    SDL_Surface *surface = chk(SDL_LoadBMP(path.c_str()));

    // RGBA32 matches R8G8B8A8_UNORM byte order on both LE and BE
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *converted = chk(SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32));
        SDL_DestroySurface(surface);
        surface = converted;
    }

    int tex_w = surface->w;
    int tex_h = surface->h;

    SDL_GPUTextureCreateInfo texInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = format,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(tex_w),
        .height = static_cast<Uint32>(tex_h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture *texture = chk(SDL_CreateGPUTexture(device_, &texInfo));

    if (!default_sampler_) {
        SDL_GPUSamplerCreateInfo sampInfo = {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        };
        default_sampler_ = chk(SDL_CreateGPUSampler(device_, &sampInfo));
    }

    // Store CPU-side pixel data (tightly packed RGBA)
    int tight_pitch = tex_w * 4;
    std::vector<Uint8> cpu_pixels(static_cast<size_t>(tight_pitch * tex_h));
    auto *src_row = static_cast<const Uint8 *>(surface->pixels);
    for (int y = 0; y < tex_h; ++y) {
        std::memcpy(cpu_pixels.data() + y * tight_pitch, src_row + y * surface->pitch, tight_pitch);
    }
    SDL_DestroySurface(surface);

    // Queue upload via Uploader (CPU data referenced, not copied)
    uploader_.Texture(texture, tex_w, tex_h, cpu_pixels.data());

    TextureHandle handle{ texture, default_sampler_, { tex_w, tex_h } };
    texture_cache_.emplace(filename, CachedTexture{ handle, std::move(cpu_pixels), tex_w, tex_h });
    return handle;
}

auto ResourceManager::LoadTextureFromSurface(SDL_Surface *surface, SDL_GPUTextureFormat format) -> TextureHandle {
    int tex_w = surface->w;
    int tex_h = surface->h;

    // Convert to RGBA32 if needed (creates a temporary surface we own)
    SDL_Surface *upload_surface = surface;
    bool own_surface = false;
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        upload_surface = chk(SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32));
        own_surface = true;
    }

    SDL_GPUTextureCreateInfo texInfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = format,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = static_cast<Uint32>(tex_w),
        .height = static_cast<Uint32>(tex_h),
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture *texture = chk(SDL_CreateGPUTexture(device_, &texInfo));

    if (!default_sampler_) {
        SDL_GPUSamplerCreateInfo sampInfo = {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        };
        default_sampler_ = chk(SDL_CreateGPUSampler(device_, &sampInfo));
    }

    // Copy pixels so we own the data until upload completes
    int tight_pitch = tex_w * 4;
    std::vector<Uint8> cpu_pixels(static_cast<size_t>(tight_pitch * tex_h));
    auto *src_row = static_cast<const Uint8 *>(upload_surface->pixels);
    for (int y = 0; y < tex_h; ++y) {
        std::memcpy(cpu_pixels.data() + y * tight_pitch, src_row + y * upload_surface->pitch, tight_pitch);
    }

    if (own_surface)
        SDL_DestroySurface(upload_surface);

    // Queue upload via Uploader (CPU data referenced, not copied)
    uploader_.Texture(texture, tex_w, tex_h, cpu_pixels.data());

    TextureHandle handle{ texture, default_sampler_, { tex_w, tex_h } };
    texture_cache_.emplace(std::format("__surface_{}x{}", tex_w, tex_h),
                           CachedTexture{ handle, std::move(cpu_pixels), tex_w, tex_h });
    return handle;
}
