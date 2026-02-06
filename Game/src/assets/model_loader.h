#pragma once
#include "assets.h"
#include <tiny_gltf.h>
#include <concepts>
#include <cgltf.h>

class ModelLoader
{
public:
    static Model LoadGLTFModel(const std::string& filename);

private:
    static Mesh LoadMesh(const cgltf_data* data, const cgltf_mesh* mesh, const std::string& basePath);
    static Texture LoadTextureFromCgltfImage(const cgltf_image* image, const std::string& basePath);
    static std::vector<uint32_t> GetIndices(const cgltf_accessor* accessor);

    template<typename T>
    static std::vector<T> GetAttributeData(const cgltf_accessor* accessor);
};
