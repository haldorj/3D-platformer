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

        std::vector<M4> inverseBind;
        if (skin.inverse_bind_matrices)
            inverseBind = GetAttributeData<M4>(skin.inverse_bind_matrices);

        std::unordered_map<int, int> nodeToJoint;
        for (size_t j = 0; j < skin.joints_count; ++j)
            nodeToJoint[int(skin.joints[j] - data->nodes)] = int(j);

        skeleton.Joints.reserve(skin.joints_count);
        for (size_t j = 0; j < skin.joints_count; ++j)
        {
            Joint joint{};
            joint.ID = int(skin.joints[j] - data->nodes);
            joint.Name = skin.joints[j]->name;
            if (!inverseBind.empty() && j < inverseBind.size())
                joint.InverseBindTransform = inverseBind[j];
            joint.Children.clear();

            const cgltf_node* node = skin.joints[j];
            for (size_t c = 0; c < node->children_count; ++c)
            {
                int childNode = int(node->children[c] - data->nodes);
                auto it = nodeToJoint.find(childNode);
                if (it != nodeToJoint.end())
                    joint.Children.push_back(it->second);
            }
            skeleton.Joints.push_back(std::move(joint));
        }

        skeleton.NodeIndexToJointID = nodeToJoint;
        skeleton.RootJoint = (skin.skeleton) ? nodeToJoint[int(skin.skeleton - data->nodes)] : 0;
        skeleton.JointCount = static_cast<uint32_t>(skin.joints_count);
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
    std::println("Success: Loaded model with {} mesh(es)", result.Meshes.size());
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
