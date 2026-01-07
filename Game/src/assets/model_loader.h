// Based on loader_example.cc from https://github.com/syoyo/tinygltf
// 
// TODO(syoyo): Print extensions and extras for each glTF object.
//
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

Texture CreateErrorTexture()
{
    Texture texture{};

    constexpr size_t width = 256;
    constexpr size_t height = 256;
    auto pixels = static_cast<unsigned char*>(malloc(width * height * 4));

    if (!pixels)
    {
        return texture;
    }

    // Checkerboard pattern
    for (auto y = 0; y < height; ++y)
    {
        for (auto x = 0; x < width; ++x)
        {
            const int index = (y * width + x) * 4;
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
    texture.Pixels = std::vector<unsigned char>(pixels, pixels + (width * height * 4));

    free(pixels);

    return texture;
}

template<typename T>
static void GetAttributeData(const tinygltf::Model& model,
    const tinygltf::Accessor& accessor,
    std::vector<T>& out)
{
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

    size_t stride = accessor.ByteStride(bufferView);
    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

    out.resize(accessor.count);
    for (size_t i = 0; i < accessor.count; ++i) {
        memcpy(&out[i], dataPtr + i * stride, sizeof(T));
    }
}

static void GetIndices(const tinygltf::Model& model,
    const tinygltf::Accessor& accessor,
    std::vector<uint32_t>& out)
{
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

    out.resize(accessor.count);

    switch (accessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
        const uint8_t* src = reinterpret_cast<const uint8_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            out[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
        const uint16_t* src = reinterpret_cast<const uint16_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            out[i] = static_cast<uint32_t>(src[i]);
        break;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
        const uint32_t* src = reinterpret_cast<const uint32_t*>(dataPtr);
        for (size_t i = 0; i < accessor.count; ++i)
            out[i] = src[i];
        break;
    }
    default:
        std::println("Unsupported index component type: {}", accessor.componentType);
        out.clear();
        break;
    }
}

static Mesh LoadMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh)
{
    Mesh mesh;

    for (const auto& primitive : gltfMesh.primitives)
    {
        const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

        std::vector<V3> positions;
        std::vector<V3> normals;
        std::vector<V2> texcoords;
        std::vector<uint32_t> indices;

        GetAttributeData(model, posAccessor, positions);
        GetAttributeData(model, normalAccessor, normals);
        GetAttributeData(model, uvAccessor, texcoords);

        mesh.Vertices.resize(positions.size());
        for (size_t i = 0; i < positions.size(); ++i)
        {
            mesh.Vertices[i].Position = positions[i];
            if (i < normals.size()) mesh.Vertices[i].Normal = normals[i];
            if (i < texcoords.size()) mesh.Vertices[i].TexCoord = texcoords[i];
        }

        GetIndices(model, indexAccessor, mesh.Indices);
    }

    return mesh;
}

static Texture LoadTexture(const tinygltf::Image& image)
{
    Texture tex;
    tex.Width = image.width;
    tex.Height = image.height;
    tex.Pixels = image.image;
    return tex;
}

bool LoadGLTFModel(const std::string& filename, Model& outModel)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;
    std::string err, warn;

    bool isBinary = (filename.substr(filename.size() - 4) == ".glb");
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
        return false;
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
            if (prim.material >= 0)
            {
                const auto& mat = gltfModel.materials[prim.material];
                int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
                if (texIndex >= 0 && texIndex < (int)textures.size())
                    mesh.Textures.push_back(textures[texIndex]);
            }
        }
        outModel.Meshes.push_back(mesh);
    }

    std::println("Loaded model: {} mesh(es)", outModel.Meshes.size());
    return true;
}
