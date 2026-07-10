#include <model_loader.hpp>
#include <scene.hpp>
#include <components.hpp>
#include <image_decoder.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL3/SDL_iostream.h>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <format>
#include <cstdint>
#include <array>
#include <variant>
#include <functional>

namespace {

auto ComputeNodeTransform(const fastgltf::Node &node) -> glm::mat4 {
    return std::visit([](const auto &arg) -> glm::mat4 {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, fastgltf::math::fmat4x4>) {
            return glm::make_mat4(arg.data());
        } else {
            const auto &trs = arg;
            glm::mat4 xform{1.0f};
            xform = glm::translate(xform, glm::vec3{trs.translation[0], trs.translation[1], trs.translation[2]});
            glm::quat q{trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]};
            xform *= glm::mat4_cast(q);
            xform = glm::scale(xform, glm::vec3{trs.scale[0], trs.scale[1], trs.scale[2]});
            return xform;
        }
    }, node.transform);
}

auto DecodeGltfImage(const fastgltf::Asset &asset, const fastgltf::Image &image) -> SDL_Surface * {
    return std::visit([&](const auto &arg) -> SDL_Surface * {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, fastgltf::sources::BufferView>) {
            auto &bv = asset.bufferViews[arg.bufferViewIndex];
            auto &buffer = asset.buffers[bv.bufferIndex];
            SDL_Log("[gltf] image: bufferView %zu (buffer %zu, offset %zu, length %zu)",
                    arg.bufferViewIndex, bv.bufferIndex, bv.byteOffset, bv.byteLength);
            return std::visit([&](const auto &bufData) -> SDL_Surface * {
                using B = std::decay_t<decltype(bufData)>;
                if constexpr (std::is_same_v<B, fastgltf::sources::Array> ||
                              std::is_same_v<B, fastgltf::sources::ByteView>) {
                    auto *data = bufData.bytes.data() + bv.byteOffset;
                    auto len = static_cast<size_t>(bv.byteLength);
                    // Try SDL first, fall back to stb_image for unsupported formats (e.g. JPEG)
                    auto *stream = SDL_IOFromConstMem(data, len);
                    if (!stream) return nullptr;
                    auto *surface = SDL_LoadSurface_IO(stream, true);
                    if (surface) return surface;
                    surface = DecodeImageBytes(reinterpret_cast<const std::byte *>(data), len);
                    return surface;
                }
                SDL_Log("[gltf] unhandled buffer data variant");
                return nullptr;
            }, buffer.data);
        } else if constexpr (std::is_same_v<T, fastgltf::sources::URI>) {
            auto fpath = arg.uri.fspath();
            auto *surface = SDL_LoadSurface(fpath.string().c_str());
            SDL_Log("[gltf] decoded URI image '%s': %s", fpath.string().c_str(), surface ? "OK" : "FAILED");
            return surface;
        } else if constexpr (std::is_same_v<T, fastgltf::sources::Array> ||
                              std::is_same_v<T, fastgltf::sources::ByteView>) {
            auto *stream = SDL_IOFromConstMem(arg.bytes.data(), arg.bytes.size());
            if (!stream) return nullptr;
            auto *surface = SDL_LoadSurface_IO(stream, true);
            SDL_Log("[gltf] decoded inline image: %s", surface ? "OK" : "FAILED");
            return surface;
        }
        SDL_Log("[gltf] unhandled image data variant");
        return nullptr;
    }, image.data);
}

auto CreateSolidColorSurface(float r, float g, float b, float a) -> SDL_Surface * {
    auto *surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        SDL_WriteSurfacePixel(surface, 0, 0,
                              static_cast<Uint8>(r * 255.0f),
                              static_cast<Uint8>(g * 255.0f),
                              static_cast<Uint8>(b * 255.0f),
                              static_cast<Uint8>(a * 255.0f));
    }
    return surface;
}

} // namespace

auto LoadGLTF(std::string_view path, Scene &scene, size_t materialBaseOffset) -> GltfLoadResult {
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None)
        return {};

    fastgltf::Parser parser;
    auto asset = parser.loadGltf(data.get(), path);
    if (asset.error() != fastgltf::Error::None)
        return {};

    auto &assetRef = asset.get();
    SDL_Log("[gltf] loaded '%s': %zu meshes, %zu materials, %zu images, %zu nodes",
            std::filesystem::path(path).filename().string().c_str(),
            assetRef.meshes.size(), assetRef.materials.size(),
            assetRef.images.size(), assetRef.nodes.size());

    // Map mesh index -> vector of { geometry index, glTF material index }
    struct PrimitiveInfo {
        size_t geometryIndex;
        size_t materialIndex;
    };
    std::unordered_map<size_t, std::vector<PrimitiveInfo>> meshToPrimitives;

    for (size_t mi = 0; mi < assetRef.meshes.size(); ++mi) {
        auto &mesh = assetRef.meshes[mi];
        SDL_Log("[gltf] mesh %zu: %zu primitives", mi, mesh.primitives.size());
        for (auto &primitive : mesh.primitives) {
            auto posAttrIt = primitive.findAttribute("POSITION");
            if (posAttrIt == primitive.attributes.end()) {
                SDL_Log("[gltf]   primitive: no POSITION -> skip");
                continue;
            }
            if (!primitive.indicesAccessor.has_value()) {
                SDL_Log("[gltf]   primitive: no indices -> skip");
                continue;
            }

            auto &posAccessor = assetRef.accessors[posAttrIt->accessorIndex];
            auto uvAttrIt = primitive.findAttribute("TEXCOORD_0");

            std::vector<PositionTextureVertex> vertices;
            vertices.reserve(posAccessor.count);

            if (uvAttrIt != primitive.attributes.end()) {
                auto &uvAccessor = assetRef.accessors[uvAttrIt->accessorIndex];
                auto posAcc = fastgltf::iterateAccessor<glm::vec3>(assetRef, posAccessor);
                auto uvAcc = fastgltf::iterateAccessor<glm::vec2>(assetRef, uvAccessor);
                auto posIt = posAcc.begin();
                auto uvIt = uvAcc.begin();
                for (size_t i = 0; i < posAccessor.count; ++i) {
                    vertices.push_back({*posIt, *uvIt});
                    ++posIt;
                    ++uvIt;
                }
            } else {
                for (auto pos : fastgltf::iterateAccessor<glm::vec3>(assetRef, posAccessor)) {
                    vertices.push_back({pos, glm::vec2{0.0f}});
                }
            }

            std::vector<Uint32> indices;
            auto &idxAccessor = assetRef.accessors[*primitive.indicesAccessor];
            indices.reserve(idxAccessor.count);

            switch (idxAccessor.componentType) {
                case fastgltf::ComponentType::UnsignedByte:
                    for (auto idx : fastgltf::iterateAccessor<std::uint8_t>(assetRef, idxAccessor))
                        indices.push_back(idx);
                    break;
                case fastgltf::ComponentType::UnsignedShort:
                    for (auto idx : fastgltf::iterateAccessor<std::uint16_t>(assetRef, idxAccessor))
                        indices.push_back(idx);
                    break;
                case fastgltf::ComponentType::UnsignedInt:
                    for (auto idx : fastgltf::iterateAccessor<std::uint32_t>(assetRef, idxAccessor))
                        indices.push_back(idx);
                    break;
                default:
                    continue;
            }

            auto geoIdx = scene.AddGeometry(vertices, indices);
            auto matIdx = primitive.materialIndex.value_or(0);
            meshToPrimitives[mi].push_back({geoIdx, matIdx});
        }
    }

    // Collect unique material indices used by any primitive
    std::unordered_set<size_t> usedMaterials;
    for (auto &[mi, prims] : meshToPrimitives) {
        for (auto &p : prims)
            usedMaterials.insert(p.materialIndex);
    }

    // Load base color texture for each used material
    GltfLoadResult result;
    size_t maxMatIdx = 0;
    for (auto idx : usedMaterials)
        if (idx > maxMatIdx) maxMatIdx = idx;
    result.materialSurfaces.resize(maxMatIdx + 1, nullptr);

    for (auto matIdx : usedMaterials) {
        if (matIdx < assetRef.materials.size()) {
            auto &mat = assetRef.materials[matIdx];
            auto &pbr = mat.pbrData;

            if (pbr.baseColorTexture.has_value()) {
                auto &texInfo = *pbr.baseColorTexture;
                if (texInfo.textureIndex < assetRef.textures.size()) {
                    auto &texture = assetRef.textures[texInfo.textureIndex];
                    SDL_Log("[gltf] mat %zu -> tex %zu: imageIndex %s", matIdx, texInfo.textureIndex,
                            texture.imageIndex.has_value() ? "yes" : "no");
                    if (texture.imageIndex.has_value() &&
                        static_cast<size_t>(*texture.imageIndex) < assetRef.images.size()) {
                        auto &image = assetRef.images[static_cast<size_t>(*texture.imageIndex)];
                        result.materialSurfaces[matIdx] = DecodeGltfImage(assetRef, image);
                    }
                }
            }

            if (!result.materialSurfaces[matIdx]) {
                auto &f = pbr.baseColorFactor;
                SDL_Log("[gltf] mat %zu: no texture, fallback to color (%.2f, %.2f, %.2f, %.2f)",
                        matIdx, f[0], f[1], f[2], f[3]);
                result.materialSurfaces[matIdx] = CreateSolidColorSurface(f[0], f[1], f[2], f[3]);
            } else {
                SDL_Log("[gltf] mat %zu: loaded texture surface (%dx%d)", matIdx,
                        result.materialSurfaces[matIdx]->w, result.materialSurfaces[matIdx]->h);
            }
        }
    }

    // Fill unused material slots with white to keep indices aligned
    for (size_t mi = 0; mi < result.materialSurfaces.size(); ++mi) {
        if (!result.materialSurfaces[mi]) {
            SDL_Log("[gltf] filling unused mat %zu with white", mi);
            result.materialSurfaces[mi] = CreateSolidColorSurface(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // Recursively process node hierarchy, accumulating transforms
    size_t entityCount = 0;
    std::function<void(const fastgltf::Node &, glm::mat4, int)> processNode;
    processNode = [&](const fastgltf::Node &node, glm::mat4 parentXform, int depth) {
        auto worldXform = parentXform * ComputeNodeTransform(node);
        if (node.meshIndex.has_value()) {
            auto mi = static_cast<size_t>(*node.meshIndex);
            SDL_Log("[gltf] node '%s': meshIndex=%zu, %zu primitives",
                    node.name.c_str(), mi,
                    meshToPrimitives.count(mi) ? meshToPrimitives[mi].size() : 0);
            auto &prims = meshToPrimitives[mi];
            for (auto &p : prims) {
                auto entity = scene.CreateEntity({p.geometryIndex}, {materialBaseOffset + p.materialIndex}, worldXform);
                scene.Registry().emplace<GlTFNode>(entity);
                ++entityCount;
            }
        } else {
            SDL_Log("[gltf] node '%s': no mesh, %zu children", node.name.c_str(), node.children.size());
        }
        for (auto childIdx : node.children)
            processNode(assetRef.nodes[childIdx], worldXform, depth + 1);
    };

    SDL_Log("[gltf] processing %zu root nodes:", assetRef.nodes.size());
    for (size_t ni = 0; ni < assetRef.nodes.size(); ++ni)
        processNode(assetRef.nodes[ni], glm::mat4{1}, 0);

    SDL_Log("[gltf] created %zu entities", entityCount);

    result.entityCount = entityCount;
    return result;
}
