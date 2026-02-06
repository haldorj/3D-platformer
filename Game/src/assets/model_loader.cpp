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

static std::string GetBasePath(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? "" : path.substr(0, pos + 1);
}

Model ModelLoader::LoadGLTFModel(const std::string& filename)
{
    std::println("Attempting to load model from file: {}", filename);

    Model result{};
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result res = cgltf_parse_file(&options, filename.c_str(), &data);
    if (res != cgltf_result_success)
    {
        std::println("Failed to parse glTF: {}", filename);
        return {};
    }

    res = cgltf_load_buffers(&options, data, filename.c_str());
    if (res != cgltf_result_success)
    {
        std::println("Failed to load glTF buffers: {}", filename);
        cgltf_free(data);
        return {};
    }

    std::string basePath = GetBasePath(filename);

    // ------------------- Load Textures -------------------
    std::println("Loading textures...");
    std::vector<Texture> textures;
    for (size_t i = 0; i < data->images_count; ++i)
    {
        Texture tex = LoadTextureFromCgltfImage(&data->images[i], basePath);
        textures.push_back(tex.Pixels.empty() ? CreateErrorTexture() : tex);
    }

    // ------------------- Load Meshes -------------------
    std::println("Loading meshes...");
    for (size_t i = 0; i < data->meshes_count; ++i)
    {
        Mesh mesh = LoadMesh(data, &data->meshes[i], basePath);

        // Assign materials/textures
        for (size_t p = 0; p < data->meshes[i].primitives_count; ++p)
        {
            const cgltf_primitive& prim = data->meshes[i].primitives[p];
            if (!prim.material) continue;
            const cgltf_material* mat = prim.material;
            if (mat->has_pbr_metallic_roughness)
            {
                const cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
                if (tex && tex->image)
                {
                    ptrdiff_t texOffset = tex->image - data->images;
                    if (texOffset >= 0 && static_cast<size_t>(texOffset) < data->images_count)
                    {
                        int texIndex = static_cast<int>(texOffset);
                        mesh.Textures.push_back(textures[texIndex]);
                    }
                }
            }
        }
        result.Meshes.push_back(std::move(mesh));
    }

    // ------------------- Load Skeletons -------------------
    std::println("Loading skins...");
    for (size_t i = 0; i < data->skins_count; ++i)
    {
        const cgltf_skin& skin = data->skins[i];
        Skeleton skeleton{};
        skeleton.Bones.resize(skin.joints_count);

        std::vector<M4> inverseBind;
        if (skin.inverse_bind_matrices)
            inverseBind = GetAttributeData<M4>(skin.inverse_bind_matrices);

        std::unordered_map<int, int> nodeToBone;
        for (size_t j = 0; j < skin.joints_count; ++j)
            nodeToBone[int(skin.joints[j] - data->nodes)] = int(j);

        for (size_t j = 0; j < skin.joints_count; ++j)
        {
            BoneInfo& bone = skeleton.Bones[j];
            bone.ID = int(skin.joints[j] - data->nodes);
            if (!inverseBind.empty() && j < inverseBind.size())
                bone.InverseBindMatrix = inverseBind[j];
            bone.Children.clear();

            const cgltf_node* node = skin.joints[j];
            for (size_t c = 0; c < node->children_count; ++c)
            {
                int childNode = int(node->children[c] - data->nodes);
                auto it = nodeToBone.find(childNode);
                if (it != nodeToBone.end())
                    bone.Children.push_back(it->second);
            }
        }

        skeleton.RootBone = (skin.skeleton) ? nodeToBone[int(skin.skeleton - data->nodes)] : 0;
        result.Skeletons.push_back(std::move(skeleton));
    }

    // ------------------- Load Animations -------------------
    std::println("Loading animations...");
    for (size_t i = 0; i < data->animations_count; ++i)
    {
        const cgltf_animation& animData = data->animations[i];
        Animation anim{};
        anim.Name = animData.name ? animData.name : "";

        for (size_t j = 0; j < animData.channels_count; ++j)
        {
            const cgltf_animation_channel& chData = animData.channels[j];
            const cgltf_animation_sampler& samp = *chData.sampler;

            AnimationChannel ch{};
            ch.TargetNode = int(chData.target_node - data->nodes);
            ch.Path = cgltf_animation_path_type_translation == chData.target_path ? "translation" :
                cgltf_animation_path_type_rotation == chData.target_path ? "rotation" :
                cgltf_animation_path_type_scale == chData.target_path ? "scale" : "unknown";

            ch.Times = GetAttributeData<float>(samp.input);
            if (ch.Path == "translation")
                ch.Translations = GetAttributeData<V3>(samp.output);
            else if (ch.Path == "rotation")
                ch.Rotations = GetAttributeData<Quat>(samp.output);
            else if (ch.Path == "scale")
                ch.Scales = GetAttributeData<V3>(samp.output);

            anim.Channels.push_back(std::move(ch));
        }

        anim.Duration = 0.0f;
        for (const auto& ch : anim.Channels)
            if (!ch.Times.empty())
                anim.Duration = std::max<float>(anim.Duration, ch.Times.back());

        result.Animations.push_back(std::move(anim));
    }

    cgltf_free(data);
    std::println("Loaded model with {} mesh(es)", result.Meshes.size());
    return result;
}

Mesh ModelLoader::LoadMesh(const cgltf_data* data, const cgltf_mesh* gltfMesh, const std::string&)
{
    Mesh result;
    for (size_t p = 0; p < gltfMesh->primitives_count; ++p)
    {
        const cgltf_primitive& prim = gltfMesh->primitives[p];
        const cgltf_accessor* pos = nullptr;
        const cgltf_accessor* normal = nullptr;
        const cgltf_accessor* uv = nullptr;
        const cgltf_accessor* joints = nullptr;
        const cgltf_accessor* weights = nullptr;

        for (size_t a = 0; a < prim.attributes_count; ++a)
        {
            const cgltf_attribute& attr = prim.attributes[a];
            switch (attr.type)
            {
            case cgltf_attribute_type_position: pos = attr.data; break;
            case cgltf_attribute_type_normal: normal = attr.data; break;
            case cgltf_attribute_type_texcoord: uv = attr.data; break;
            case cgltf_attribute_type_joints: joints = attr.data; break;
            case cgltf_attribute_type_weights: weights = attr.data; break;
            default: break;
            }
        }

        std::vector<V3> positions = pos ? GetAttributeData<V3>(pos) : std::vector<V3>{};
        std::vector<V3> normals = normal ? GetAttributeData<V3>(normal) : std::vector<V3>{};
        std::vector<V2> texcoords = uv ? GetAttributeData<V2>(uv) : std::vector<V2>{};
        std::vector<IV4> jointData = joints ? GetAttributeData<IV4>(joints) : std::vector<IV4>{};
        std::vector<V4> weightData = weights ? GetAttributeData<V4>(weights) : std::vector<V4>{};

        result.Vertices.resize(positions.size());
        for (size_t i = 0; i < positions.size(); ++i)
        {
            Vertex& v = result.Vertices[i];
            v.Position = positions[i];
            if (i < normals.size()) v.Normal = normals[i];
            if (i < texcoords.size()) v.TexCoord = texcoords[i];
            if (i < jointData.size()) v.BoneIDs = jointData[i];
            if (i < weightData.size()) v.Weights = weightData[i];
        }

        if (prim.indices)
            result.Indices = GetIndices(prim.indices);
    }
    return result;
}

Texture ModelLoader::LoadTextureFromCgltfImage(const cgltf_image* image, const std::string& basePath)
{
    Texture result{};
    if (image->buffer_view)
    {
        const cgltf_buffer_view* view = image->buffer_view;
        const unsigned char* dataPtr = (const unsigned char*)view->buffer->data + view->offset;
        size_t dataSize = view->size;

        int w, h, comp;
        unsigned char* data = stbi_load_from_memory(dataPtr, (int)dataSize, &w, &h, &comp, STBI_rgb_alpha);
        if (data)
        {
            result.Width = w;
            result.Height = h;
            result.Pixels.assign(data, data + w * h * 4);
            stbi_image_free(data);
        }
        else
        {
            result = CreateErrorTexture();
        }
    }
    else if (image->uri)
    {
        std::string fullPath = basePath + image->uri;
        result = LoadTextureFromFile(fullPath);
    }
    else
    {
        result = CreateErrorTexture();
    }
    return result;
}

std::vector<uint32_t> ModelLoader::GetIndices(const cgltf_accessor* accessor)
{
    std::vector<uint32_t> result(accessor->count);
    for (size_t i = 0; i < accessor->count; ++i)
        result[i] = (uint32_t)cgltf_accessor_read_index(accessor, i);
    return result;
}

template<typename T>
std::vector<T> ModelLoader::GetAttributeData(const cgltf_accessor* accessor)
{
    std::vector<T> result(accessor->count);
    for (size_t i = 0; i < accessor->count; ++i)
        cgltf_accessor_read_float(accessor, i, reinterpret_cast<float*>(&result[i]), sizeof(T) / sizeof(float));
    return result;
}

//Model ModelLoader::LoadGLTFModel(const std::string& filename)
//{
//    std::print("Attempting to load model from file: \n\t{}.\n", filename);
//
//    Model result{};
//
//    tinygltf::TinyGLTF loader;
//    tinygltf::Model gltfModel;
//    std::string err{}, warn{};
//
//    std::string ext = GetFilePathExtension(filename);
//
//    bool isBinary = (ext == "glb");
//    bool ok = false;
//
//    if (isBinary)
//    {
//        ok = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filename);
//        std::println("loaded .glb from file.");
//    }
//    else
//    {
//        ok = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
//        std::println("loaded .{} from file.", ext);
//    }
//
//    if (!warn.empty()) 
//    {
//        std::println("Warning: {}", warn);
//    }
//    if (!err.empty())
//    {
//        std::println("Error: {}", err);
//    }
//    if (!ok)
//    {
//        std::println("Failed to load glTF: {}", filename);
//        return {};
//    }
//
//    std::println("loading textures...");
//
//    std::vector<Texture> textures;
//    for (auto& img : gltfModel.images)
//    {
//        Texture tex = LoadTexture(img);
//        if (tex.Pixels.size() > 0)
//        {
//            textures.push_back(tex);
//        }
//        else
//        {
//            std::println("Failed to load texture image, using error texture instead.");
//            textures.push_back(CreateErrorTexture());
//        }
//    }
//
//    std::println("loading meshes...");
//
//    for (auto& gltfMesh : gltfModel.meshes)
//    {
//        Mesh mesh = LoadMesh(gltfModel, gltfMesh);
//        for (auto& prim : gltfMesh.primitives)
//        {
//            if (prim.material < 0)
//                continue;
//
//            const auto& mat = gltfModel.materials[prim.material];
//            int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
//
//            if (texIndex >= 0 && texIndex < static_cast<int>(textures.size()))
//            {
//                mesh.Textures.push_back(textures[texIndex]);
//            }
//        }
//        result.Meshes.push_back(std::move(mesh));
//    }
//
//    for (auto& gltfSkin : gltfModel.skins)
//    {
//        const auto& matrixAccessor = gltfModel.accessors[gltfSkin.inverseBindMatrices];
//        std::vector<M4> inverseBindMatrices = GetAttributeData<M4>(gltfModel, matrixAccessor);
//
//        Skeleton skeleton{};
//        skeleton.Bones.resize(gltfSkin.joints.size());
//
//        std::unordered_map<int, int> nodeToBoneIndex;
//        for (size_t i = 0; i < gltfSkin.joints.size(); ++i)
//            nodeToBoneIndex[gltfSkin.joints[i]] = static_cast<int>(i);
//
//        for (size_t i = 0; i < gltfSkin.joints.size(); ++i)
//        {
//            int nodeIndex = gltfSkin.joints[i];
//            BoneInfo& bone = skeleton.Bones[i];
//            bone.ID = nodeIndex;
//            bone.OffsetMatrix = inverseBindMatrices[i];
//            bone.Children.clear();
//
//            const auto& node = gltfModel.nodes[nodeIndex];
//            for (int childNode : node.children)
//            {
//                auto it = nodeToBoneIndex.find(childNode);
//                if (it != nodeToBoneIndex.end())
//                    bone.Children.push_back(it->second);
//            }
//        }
//
//        skeleton.RootBone = gltfSkin.skeleton >= 0
//            ? nodeToBoneIndex[gltfSkin.skeleton]
//            : 0;
//
//        result.Skeletons.push_back(std::move(skeleton));
//    }
//
//    std::println("loading animations...");
//    for (auto& gltfAnim : gltfModel.animations)
//    {
//        Animation anim{};
//        anim.Name = gltfAnim.name;
//
//        for (const auto& channel : gltfAnim.channels)
//        {
//            const auto& sampler = gltfAnim.samplers[channel.sampler];
//            AnimationChannel ch{};
//            
//            // The target node, usually refers to the bone.
//            ch.TargetNode = channel.target_node;
//
//            // Path: name of the animated property.
//            // Can be "translation", "rotation" or "scale".
//            ch.Path = channel.target_path; 
//
//            // Time input, the times of the key frames.
//            const auto& timeAccessor = gltfModel.accessors[sampler.input];
//            ch.Times = GetAttributeData<float>(gltfModel, timeAccessor);
//
//            // Value output for a given key frame.
//            const auto& valueAccessor = gltfModel.accessors[sampler.output];
//            if (ch.Path == "translation")
//                ch.Translations = GetAttributeData<V3>(gltfModel, valueAccessor);
//            else if (ch.Path == "rotation")
//                ch.Rotations = GetAttributeData<Quat>(gltfModel, valueAccessor);
//            else if (ch.Path == "scale")
//                ch.Scales = GetAttributeData<V3>(gltfModel, valueAccessor);
//
//            anim.Channels.push_back(std::move(ch));
//        }
//
//        anim.Duration = 0.0f;
//        for (const auto& ch : anim.Channels)
//            if (!ch.Times.empty())
//                anim.Duration = std::max<float>(anim.Duration, ch.Times.back());
//
//        result.Animations.push_back(std::move(anim));
//    }
//
//    std::println("Loaded model: {} mesh(es)", result.Meshes.size());
//
//    return result;
//}
//
//Mesh ModelLoader::LoadMesh(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh)
//{
//    Mesh result;
//
//    for (const auto& primitive : gltfMesh.primitives)
//    {
//        const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
//        const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
//        const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
//        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
//
//        // The JOINTS_0 attribute data contains the indices of the joints that should affect the vertex.
//        const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
//        // The WEIGHTS_0 attribute data defines the weights indicating how strongly 
//        // the joint should influence the vertex.
//        const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
//
//        std::vector<V3> positions = GetAttributeData<V3>(model, posAccessor);
//        std::vector<V3> normals = GetAttributeData<V3>(model, normalAccessor);
//        std::vector<V2> texcoords = GetAttributeData<V2>(model, uvAccessor);
//        std::vector<IV4> joints = GetAttributeData<IV4>(model, jointAccessor);
//        std::vector<V4> weights = GetAttributeData<V4>(model, weightAccessor);
//
//        result.Vertices.resize(positions.size());
//        for (size_t i = 0; i < positions.size(); ++i)
//        {
//            Vertex& v = result.Vertices[i];
//            v.Position = positions[i];
//            if (i < normals.size()) v.Normal = normals[i];
//            if (i < texcoords.size()) v.TexCoord = texcoords[i];
//            if (i < joints.size()) v.BoneIDs = joints[i];
//            if (i < weights.size()) v.Weights = weights[i];
//        }
//
//        result.Indices = GetIndices(model, indexAccessor);
//    }
//
//    return result;
//}
//
//Texture ModelLoader::LoadTexture(const tinygltf::Image& image)
//{
//    Texture result;
//    result.Width = image.width;
//    result.Height = image.height;
//    result.Pixels = image.image;
//    return result;
//}
//
//std::vector<uint32_t> ModelLoader::GetIndices(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
//{
//    std::vector<uint32_t> result;
//
//    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
//    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
//    const unsigned char* dataPtr = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
//
//    result.resize(accessor.count);
//
//    switch (accessor.componentType)
//    {
//    case TINYGLTF_COMPONENT_TYPE_BYTE:
//    {
//        const int8_t* src = reinterpret_cast<const int8_t*>(dataPtr);
//        for (size_t i = 0; i < accessor.count; ++i)
//            result[i] = static_cast<uint32_t>(src[i]);
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
//    {
//        const uint8_t* src = reinterpret_cast<const uint8_t*>(dataPtr);
//        for (size_t i = 0; i < accessor.count; ++i)
//            result[i] = static_cast<uint32_t>(src[i]);
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_SHORT:
//    {
//        const int16_t* src = reinterpret_cast<const int16_t*>(dataPtr);
//        for (size_t i = 0; i < accessor.count; ++i)
//            result[i] = static_cast<uint32_t>(src[i]);
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
//    {
//        const uint16_t* src = reinterpret_cast<const uint16_t*>(dataPtr);
//        for (size_t i = 0; i < accessor.count; ++i)
//            result[i] = static_cast<uint32_t>(src[i]);
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_INT:
//    {
//        const int32_t* src = reinterpret_cast<const int32_t*>(dataPtr);
//        for (size_t i = 0; i < accessor.count; ++i)
//            result[i] = static_cast<uint32_t>(src[i]);
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
//    {
//        const uint32_t* src = reinterpret_cast<const uint32_t*>(dataPtr);
//        for (size_t i = 0; i < accessor.count; ++i)
//            result[i] = src[i];
//        break;
//    }
//    default:
//    {
//        std::println("Unsupported index component type: {}", accessor.componentType);
//        result.clear();
//        Assert(false);
//        break;
//    }
//    }
//
//    return result;
//}
//
//void ModelLoader::PrintComponentType(int componentType)
//{
//    switch (componentType)
//    {
//    case TINYGLTF_COMPONENT_TYPE_BYTE:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_BYTE");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_SHORT:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_SHORT");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_INT:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_INT");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_UNSIGNED_INT");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_FLOAT:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_FLOAT");
//        break;
//    }
//    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
//    {
//        std::println("\tTINYGLTF_COMPONENT_TYPE_DOUBLE");
//        break;
//    }
//    default:
//    {
//        Assert(false);
//        break;
//    }
//
//    }
//}
