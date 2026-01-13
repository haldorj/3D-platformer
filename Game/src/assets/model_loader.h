#pragma once
#include "assets.h"
#include <tiny_gltf.h>

class ModelLoader
{
public:
    static Model LoadGLTFModel(const std::string& filename);

private:
    static Mesh LoadMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh);
    static Texture LoadTexture(const tinygltf::Image& image);
    static std::vector<uint32_t> GetIndices(
        const tinygltf::Model& model, const tinygltf::Accessor& accessor);

    static std::vector<V2> GetV2AttributeData(
        const tinygltf::Model& model, const tinygltf::Accessor& accessor);

    static std::vector<V3> GetV3AttributeData(
        const tinygltf::Model& model, const tinygltf::Accessor& accessor);
};
