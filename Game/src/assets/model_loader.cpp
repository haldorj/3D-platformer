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
    stbi_image_free(data);
    return texture;
}

Model ModelLoader::LoadGLTFModel(const std::string& filename)
{
    std::print("Attempting to load model from file: \n\t{}.\n", filename);

    Model result{};

    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;
    std::string err{}, warn{};

    std::string ext = GetFilePathExtension(filename);

    bool isBinary = (ext == "glb");
    bool ok = false;

    if (isBinary)
    {
        ok = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename);
        std::println("loaded .glb from file.");
    }
    else
    {
        ok = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
        std::println("loaded .{} from file.", ext);
    }

    if (!warn.empty()) 
    {
        std::println("Warning: {}", warn);
    }
    if (!err.empty())
    {
        std::println("Error: {}", err);
    }
    if (!ok)
    {
        std::println("Failed to load glTF: {}", filename);
        return {};
    }

    std::println("loading textures...");

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

    std::println("loading meshes...");

    for (auto& gltfMesh : gltfModel.meshes)
    {
        Mesh mesh = LoadMesh(gltfModel, gltfMesh);
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
        result.Meshes.push_back(std::move(mesh));
    }

    for (auto& gltfSkin : gltfModel.skins)
    {
        const auto& matrixAccessor = gltfModel.accessors[gltfSkin.inverseBindMatrices];
        std::vector<M4> inverseBindMatrices = GetAttributeData<M4>(gltfModel, matrixAccessor);

        Skeleton skeleton{};
        skeleton.Bones.resize(gltfSkin.joints.size());

        std::unordered_map<int, int> nodeToBoneIndex;
        for (size_t i = 0; i < gltfSkin.joints.size(); ++i)
            nodeToBoneIndex[gltfSkin.joints[i]] = static_cast<int>(i);

        for (size_t i = 0; i < gltfSkin.joints.size(); ++i)
        {
            int nodeIndex = gltfSkin.joints[i];
            BoneInfo& bone = skeleton.Bones[i];
            bone.ID = nodeIndex;
            bone.OffsetMatrix = inverseBindMatrices[i];
            bone.Children.clear();

            const auto& node = gltfModel.nodes[nodeIndex];
            for (int childNode : node.children)
            {
                auto it = nodeToBoneIndex.find(childNode);
                if (it != nodeToBoneIndex.end())
                    bone.Children.push_back(it->second);
            }
        }

        skeleton.RootBone = gltfSkin.skeleton >= 0
            ? nodeToBoneIndex[gltfSkin.skeleton]
            : 0;

        result.Skeletons.push_back(std::move(skeleton));
    }

    std::println("loading animations...");
    for (auto& gltfAnim : gltfModel.animations)
    {
        Animation anim{};
        anim.Name = gltfAnim.name;

        for (const auto& channel : gltfAnim.channels)
        {
            const auto& sampler = gltfAnim.samplers[channel.sampler];
            AnimationChannel ch{};
            
            // The target node, usually refers to the bone.
            ch.TargetNode = channel.target_node;

            // Path: name of the animated property.
            // Can be "translation", "rotation" or "scale".
            ch.Path = channel.target_path; 

            // Time input, the times of the key frames.
            const auto& timeAccessor = gltfModel.accessors[sampler.input];
            ch.Times = GetAttributeData<float>(gltfModel, timeAccessor);

            // Value output for a given key frame.
            const auto& valueAccessor = gltfModel.accessors[sampler.output];
            if (ch.Path == "translation")
                ch.Translations = GetAttributeData<V3>(gltfModel, valueAccessor);
            else if (ch.Path == "rotation")
                ch.Rotations = GetAttributeData<Quat>(gltfModel, valueAccessor);
            else if (ch.Path == "scale")
                ch.Scales = GetAttributeData<V3>(gltfModel, valueAccessor);

            anim.Channels.push_back(std::move(ch));
        }

        anim.Duration = 0.0f;
        for (const auto& ch : anim.Channels)
            if (!ch.Times.empty())
                anim.Duration = std::max<float>(anim.Duration, ch.Times.back());

        result.Animations.push_back(std::move(anim));
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

        // The JOINTS_0 attribute data contains the indices of the joints that should affect the vertex.
        const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
        // The WEIGHTS_0 attribute data defines the weights indicating how strongly 
        // the joint should influence the vertex.
        const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];

        std::vector<V3> positions = GetAttributeData<V3>(model, posAccessor);
        std::vector<V3> normals = GetAttributeData<V3>(model, normalAccessor);
        std::vector<V2> texcoords = GetAttributeData<V2>(model, uvAccessor);
        std::vector<IV4> joints = GetAttributeData<IV4>(model, jointAccessor);
        std::vector<V4> weights = GetAttributeData<V4>(model, weightAccessor);

        result.Vertices.resize(positions.size());
        for (size_t i = 0; i < positions.size(); ++i)
        {
            Vertex& v = result.Vertices[i];
            v.Position = positions[i];
            if (i < normals.size()) v.Normal = normals[i];
            if (i < texcoords.size()) v.TexCoord = texcoords[i];
            if (i < joints.size()) v.BoneIDs = joints[i];
            if (i < weights.size()) v.Weights = weights[i];
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
        Assert(false);
        break;
    }
    }

    return result;
}

void ModelLoader::PrintComponentType(int componentType)
{
    switch (componentType)
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
}
