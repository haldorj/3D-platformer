#include <pch.h>

#include "model_loader.h"
#include "math/handmade_math.h"
#include "stb_image.h"

static Texture CreateErrorTexture()
{
    Texture texture{};

    constexpr size_t width = 256;
    constexpr size_t height = 256;
    std::vector<unsigned char> pixels{};
    pixels.resize(width * height * 4);

    // Checkerboard pattern
    for (auto y = 0; y < height; ++y)
    {
        for (auto x = 0; x < width; ++x)
        {
            const size_t index = (y * width + x) * 4;
            constexpr int checkSize = 16;

            if (x / checkSize % 2 == y / checkSize % 2)
            {
                pixels[index + 0] = 255; // R
                pixels[index + 1] = 0;   // G
                pixels[index + 2] = 255; // B
                pixels[index + 3] = 255; // A
            }
            else
            {
                pixels[index + 0] = 0;   // R
                pixels[index + 1] = 0;   // G
                pixels[index + 2] = 0;   // B
                pixels[index + 3] = 255; // A
            }
        }
    }

    texture.Width = width;
    texture.Height = height;
    texture.Pixels = pixels;

    return texture;
}

static std::string GetFilePathExtension(const std::string& FileName) {
    if (FileName.find_last_of(".") != std::string::npos)
        return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
}

static Texture LoadTextureFromFile(const std::string& path)
{
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data)
    {
        return CreateErrorTexture();
    }

    Texture texture;
    texture.Width = width;
    texture.Height = height;
    texture.Pixels = std::vector<unsigned char>(data, data + (width * height * 4));
    return texture;
}

Model ModelLoader::LoadGLTFModel(const std::string& filename)
{
    Model result;

    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;
    std::string err, warn;

    std::string ext = GetFilePathExtension(filename);

    bool isBinary = (ext == "glb");
    bool ok = false;

    if (isBinary)
        ok = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename);
    else
        ok = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);

    if (!warn.empty()) std::println("Warning: {}", warn);
    if (!err.empty())  std::println("Error: {}", err);
    if (!ok)
    {
        std::println("Failed to load glTF: {}", filename);
        return {};
    }

    // Load all textures
    std::vector<Texture> textures;
    for (auto& img : gltfModel.images)
    {
        Texture tex = LoadTexture(img);
        if (tex.Pixels.size() > 0)
        {
            textures.push_back(tex);
        }
        else
        {
            std::println("Failed to load texture image, using error texture instead.");
            textures.push_back(CreateErrorTexture());
        }
    }

    // Load all meshes
    for (auto& gltfMesh : gltfModel.meshes)
    {
        Mesh mesh = LoadMesh(gltfModel, gltfMesh);

        // Attach textures if referenced
        for (auto& prim : gltfMesh.primitives)
        {
            if (prim.material < 0)
                continue;

            const auto& mat = gltfModel.materials[prim.material];
            int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;

            if (texIndex >= 0 && texIndex < static_cast<int>(textures.size()))
            {
                mesh.Textures.push_back(textures[texIndex]);
            }
        }
        result.Meshes.push_back(mesh);
    }

    std::println("Loaded model: {} mesh(es)", result.Meshes.size());

    return result;
}

Mesh ModelLoader::LoadMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh)
{
    Mesh result;

    for (const auto& primitive : gltfMesh.primitives)
    {
        const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

        std::vector<V3> positions = GetAttributeData<V3>(model, posAccessor);
        std::vector<V3> normals = GetAttributeData<V3>(model, normalAccessor);
        std::vector<V2> texcoords = GetAttributeData<V2>(model, uvAccessor);

        result.Vertices.resize(positions.size());
        for (size_t i = 0; i < positions.size(); ++i)
        {
            result.Vertices[i].Position = positions[i];

            if (i < normals.size())
                result.Vertices[i].Normal = normals[i];

            if (i < texcoords.size())
                result.Vertices[i].TexCoord = texcoords[i];
        }

        result.Indices = GetIndices(model, indexAccessor);
    }

    return result;
}

Texture ModelLoader::LoadTexture(const tinygltf::Image& image)
{
    Texture result;
    result.Width = image.width;
    result.Height = image.height;
    result.Pixels = image.image;
    return result;
}

std::vector<uint32_t> ModelLoader::GetIndices(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
    std::vector<uint32_t> result;

    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

    result.resize(accessor.count);

    switch (accessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_BYTE:
    {
        const int8_t* src = reinterpret_cast<const int8_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            result[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            result[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_SHORT:
    {
        const int16_t* src = reinterpret_cast<const int16_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            result[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    {
        const uint16_t* src = reinterpret_cast<const uint16_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            result[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_INT:
    {
        const int32_t* src = reinterpret_cast<const int32_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            result[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    {
        const uint32_t* src = reinterpret_cast<const uint32_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            result[i] = src[i];
        break;
    }
    default:
    {
        std::println("Unsupported index component type: {}", accessor.componentType);
        result.clear();
        break;
    }
    }

    return result;
}
