#include <model_loader.hpp>
#include <scene.hpp>
#include <components.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <unordered_map>
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

} // namespace

auto LoadGLTF(std::string_view path, Scene &scene) -> size_t {
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None)
        return 0;

    fastgltf::Parser parser;
    auto asset = parser.loadGltf(data.get(), path);
    if (asset.error() != fastgltf::Error::None)
        return 0;

    auto &assetRef = asset.get();

    // Map mesh index -> vector of geometry indices (one per primitive)
    std::unordered_map<size_t, std::vector<size_t>> meshToGeometries;

    for (size_t mi = 0; mi < assetRef.meshes.size(); ++mi) {
        auto &mesh = assetRef.meshes[mi];
        for (auto &primitive : mesh.primitives) {
            auto posAttrIt = primitive.findAttribute("POSITION");
            if (posAttrIt == primitive.attributes.end()) continue;
            if (!primitive.indicesAccessor.has_value()) continue;

            auto &posAccessor = assetRef.accessors[posAttrIt->accessorIndex];
            auto uvAttrIt = primitive.findAttribute("TEXCOORD_0");

            // Build vertices
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

            // Build indices
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
            meshToGeometries[mi].push_back(geoIdx);
        }
    }

    // Recursively process node hierarchy, accumulating transforms
    size_t entityCount = 0;
    std::function<void(const fastgltf::Node &, glm::mat4)> processNode;
    processNode = [&](const fastgltf::Node &node, glm::mat4 parentXform) {
        auto worldXform = parentXform * ComputeNodeTransform(node);
        if (node.meshIndex.has_value()) {
            auto &geoIndices = meshToGeometries[static_cast<size_t>(*node.meshIndex)];
            for (auto geoIdx : geoIndices) {
                auto entity = scene.CreateEntity({geoIdx}, {0}, worldXform);
                scene.Registry().emplace<GlTFNode>(entity);
                ++entityCount;
            }
        }
        for (auto childIdx : node.children)
            processNode(assetRef.nodes[childIdx], worldXform);
    };

    for (auto &node : assetRef.nodes)
        processNode(node, glm::mat4{1});

    return entityCount;
}
