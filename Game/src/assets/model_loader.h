#pragma once
#include "assets.h"
#include <tiny_gltf.h>
#include <concepts>

class ModelLoader
{
public:
    static Model LoadGLTFModel(const std::string& filename);

private:
    static Mesh LoadMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh);
    static Texture LoadTexture(const tinygltf::Image& image);
    static std::vector<uint32_t> GetIndices(
        const tinygltf::Model& model, const tinygltf::Accessor& accessor);

    template<typename T>
    static std::vector<T>GetAttributeData(
        const tinygltf::Model& model, const tinygltf::Accessor& accessor)
    {
        switch (accessor.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_BYTE");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_SHORT");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_INT:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_INT");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_UNSIGNED_INT");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_FLOAT");
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        {
            std::println("\tTINYGLTF_COMPONENT_TYPE_DOUBLE");
            break;
        }
        default:
        {
            Assert(false);
            break;
        }
   
        }

        std::vector<T> result;
        result.reserve(accessor.count);

        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        size_t stride = accessor.ByteStride(bufferView);

        const unsigned char* dataPtr =
            buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

        result.resize(accessor.count);
        for (size_t i = 0; i < accessor.count; ++i)
        {
            std::memcpy(&result[i], dataPtr + i * stride, sizeof(T));
        }

        return result;
    }
};
