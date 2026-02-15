#include <pch.h>

#include "model_loader.h"
#include "math/handmade_math.h"
#include "assets.h"
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
            inverseBind = GetAttributeDataFloat<M4>(skin.inverse_bind_matrices);

        std::unordered_map<int, int> nodeToJoint;
        nodeToJoint.reserve(skin.joints_count);
        for (size_t j = 0; j < skin.joints_count; ++j)
        {
            nodeToJoint[int(skin.joints[j] - data->nodes)] = int(j);
        }
        skeleton.JointIDToArrayIndex = nodeToJoint;
        skeleton.Joints.resize(skin.joints_count);
        for (size_t j = 0; j < skin.joints_count; ++j)
        {
            Joint joint{};
            joint.ID = int(skin.joints[j] - data->nodes);
            joint.Name = skin.joints[j]->name;

            if (!inverseBind.empty() && j < inverseBind.size())
                joint.InverseBindTransform = inverseBind[j];

            const cgltf_node* node = skin.joints[j];
            if (node->parent)
            {
                joint.Parent = int(node->parent - data->nodes);
            }

            skeleton.Joints[j] = (std::move(joint));
        }
        result.Skeletons.push_back(std::move(skeleton));
    }

    // ------------------- Load Animations -------------------
    std::println("Loading animations...");
    for (size_t i = 0; i < data->animations_count; ++i)
    {
        const cgltf_animation& animData = data->animations[i];
        Animation anim{};
        anim.Name = animData.name ? animData.name : "";

        // Loop over animation channels
        for (size_t j = 0; j < animData.channels_count; ++j)
        {
            const cgltf_animation_channel& channel = animData.channels[j];
            const cgltf_animation_sampler& sampler = *channel.sampler;

            // Get target joint index
            if (!channel.target_node) continue; // skip non-joint animations
            int targetNode = int(channel.target_node - data->nodes);
            JointAnimation& jointAnim = anim.PerJointAnimationPoses[targetNode];
            jointAnim.TargetNode = channel.target_node->name;

            // --- Load keyframe times ---
            const cgltf_accessor* inputAccessor = sampler.input;
            std::vector<float> times(inputAccessor->count);
            cgltf_accessor_unpack_floats(inputAccessor, times.data(), inputAccessor->count);

            // --- Load keyframe values ---
            const cgltf_accessor* outputAccessor = sampler.output;

            switch (channel.target_path)
            {
            case cgltf_animation_path_type_translation:
            {
                std::vector<float> values(outputAccessor->count * 3);
                cgltf_accessor_unpack_floats(outputAccessor, values.data(), values.size());

                for (size_t k = 0; k < outputAccessor->count; ++k)
                {
                    KeyframeV3 kf{};
                    kf.Time = times[k];
                    kf.Value = { values[k * 3 + 0], values[k * 3 + 1], values[k * 3 + 2] };
                    jointAnim.Translations.push_back(kf);
                }
                break;
            }

            case cgltf_animation_path_type_rotation:
            {
                std::vector<float> values(outputAccessor->count * 4);
                cgltf_accessor_unpack_floats(outputAccessor, values.data(), values.size());

                for (size_t k = 0; k < outputAccessor->count; ++k)
                {
                    KeyframeQuat kf{};
                    kf.Time = times[k];
                    kf.Value = Quat{
                        values[k * 4 + 0],
                        values[k * 4 + 1],
                        values[k * 4 + 2],
                        values[k * 4 + 3]
                    };

                    kf.Value = NormalizeQuat(kf.Value);
                    jointAnim.Rotations.push_back(kf);
                }
                break;
            }

            case cgltf_animation_path_type_scale:
            {
                std::vector<float> values(outputAccessor->count * 3);
                cgltf_accessor_unpack_floats(outputAccessor, values.data(), values.size());

                for (size_t k = 0; k < outputAccessor->count; ++k)
                {
                    KeyframeV3 kf{};
                    kf.Time = times[k];
                    kf.Value = { values[k * 3 + 0], values[k * 3 + 1], values[k * 3 + 2] };
                    jointAnim.Scales.push_back(kf);
                }
                break;
            }

            default:
                // weights/morph targets not handled here
                break;
            }

            // Track the maximum time for animation duration
            if (!times.empty())
                anim.Duration = std::max<float>(anim.Duration, times.back());
        }
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

        std::vector<V3> positions = pos ? GetAttributeDataFloat<V3>(pos) : std::vector<V3>{};
        std::vector<V3> normals = normal ? GetAttributeDataFloat<V3>(normal) : std::vector<V3>{};
        std::vector<V2> texcoords = uv ? GetAttributeDataFloat<V2>(uv) : std::vector<V2>{};
        std::vector<IV4> jointData = joints ? GetAttributeDataUINT<IV4>(joints) : std::vector<IV4>{};
        std::vector<V4> weightData = weights ? GetAttributeDataFloat<V4>(weights) : std::vector<V4>{};

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
std::vector<T> ModelLoader::GetAttributeDataFloat(const cgltf_accessor* accessor)
{
    std::vector<T> result(accessor->count);
    for (size_t i = 0; i < accessor->count; ++i)
        cgltf_accessor_read_float(accessor, i, reinterpret_cast<float*>(&result[i]), sizeof(T) / sizeof(float));
    return result;
}

template<typename T>
std::vector<T> ModelLoader::GetAttributeDataUINT(const cgltf_accessor* accessor)
{
    std::vector<T> result(accessor->count);
    for (size_t i = 0; i < accessor->count; ++i)
        cgltf_accessor_read_uint(accessor, i, reinterpret_cast<unsigned int*>(&result[i]), sizeof(T) / sizeof(unsigned int));
    return result;
}

