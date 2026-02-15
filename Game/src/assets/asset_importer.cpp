#include "pch.h"
#include "asset_importer.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

void AssetImporter::LoadModel(std::string path, Model& model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::println("ERROR::ASSIMP:: {}", importer.GetErrorString());
        return;
    }
    _Directory = path.substr(0, path.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene, model);
}

void AssetImporter::LoadAnimation(const std::string& animationPath, Model& model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);

    for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
    {
        auto sceneAnimation = scene->mAnimations[0];

        Animation animation{};
        animation.m_Duration = static_cast<float>(sceneAnimation->mDuration);
        animation.m_TicksPerSecond = static_cast<int>(sceneAnimation->mTicksPerSecond);
        ReadHeirarchyData(animation.m_RootNode, scene->mRootNode);
        ReadMissingBones(sceneAnimation, model, animation);
    }
}

void AssetImporter::ReadMissingBones(const aiAnimation* animation, Model& model, Animation& anim)
{
    int size = animation->mNumChannels;

    auto& boneInfoMap = model.BoneInfoMap;//getting m_BoneInfoMap from Model class
    int& boneCount = model.BoneCounter; //getting the m_BoneCounter from Model class

    //reading channels(bones engaged in an animation and their keyframes)
    for (int i = 0; i < size; i++)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            boneInfoMap[boneName].id = boneCount;
            boneCount++;
        }

        Bone bone;

        bone = LoadBone(channel->mNodeName.data,
            boneInfoMap[channel->mNodeName.data].id, channel);
    }

    model.BoneInfoMap = boneInfoMap;
}

void AssetImporter::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = TransposeAndConvertMatrix(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (unsigned int i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData newData;
        ReadHeirarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

void AssetImporter::ProcessNode(aiNode* node, const aiScene* scene, Model& model)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene, model);
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, model);
    }
}

void AssetImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene, Model& model)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        V3 vector3{};
        vector3.X = mesh->mVertices[i].x;
        vector3.Y = mesh->mVertices[i].y;
        vector3.Z = mesh->mVertices[i].z;
        vertex.Position = vector3;

        vector3.X = mesh->mNormals[i].x;
        vector3.Y = mesh->mNormals[i].y;
        vector3.Z = mesh->mNormals[i].z;
        vertex.Normal = vector3;

        V2 vector2{};
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            vector2.X = mesh->mTextureCoords[0][i].x;
            vector2.Y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoord = vector2;
        }

        vertices.push_back(vertex);
    }
    // process indices
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // process bone information
    ExtractBoneWeightForVertices(vertices, mesh, scene, model);
    // process material
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps = LoadMaterialTextures(
            material, aiTextureType_DIFFUSE, "texture_diffuse");

        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = LoadMaterialTextures(material,
            aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    Mesh result{};
    result.Vertices = vertices;
    result.Indices = indices;
    result.Textures = textures;

    model.Meshes.push_back(result);
}

std::vector<Texture> AssetImporter::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        auto path = _Directory + '/' + str.C_Str();
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!data)
        {
            return {};
        }

        Texture texture;
        texture.Width = width;
        texture.Height = height;
        texture.Pixels = std::vector<unsigned char>(data, data + (width * height * 4));
        textures.push_back(texture);
        stbi_image_free(data);
    }
    return textures;
}

static void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
{
    //for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    //{
    //    if (vertex.BoneIDs[i] < 0)
    //    {
    //        vertex.Weights[i] = weight;
    //        vertex.m_BoneIDs[i] = boneID;
    //        break;
    //    }
    //}
    vertex.Weights = { weight ,weight ,weight, weight };
    vertex.BoneIDs = { boneID ,boneID ,boneID, boneID };
}

M4 AssetImporter::TransposeAndConvertMatrix(const aiMatrix4x4& from)
{
    M4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to.M[0][0] = from.a1; to.M[1][0] = from.a2; to.M[2][0] = from.a3; to.M[3][0] = from.a4;
    to.M[0][1] = from.b1; to.M[1][1] = from.b2; to.M[2][1] = from.b3; to.M[3][1] = from.b4;
    to.M[0][2] = from.c1; to.M[1][2] = from.c2; to.M[2][2] = from.c3; to.M[3][2] = from.c4;
    to.M[0][3] = from.d1; to.M[1][3] = from.d2; to.M[2][3] = from.d3; to.M[3][3] = from.d4;
    return to;
}

Bone AssetImporter::LoadBone(const std::string& name, int ID, const aiNodeAnim* channel)
{
    Bone bone{};

    bone.m_NumPositions = channel->mNumPositionKeys;

    for (int positionIndex = 0; positionIndex < bone.m_NumPositions; ++positionIndex)
    {
        aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
        float timeStamp = 
            static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
        KeyPosition data{};
        data.position.X = aiPosition.x;
        data.position.Y = aiPosition.y;
        data.position.Z = aiPosition.z;
        data.timeStamp = timeStamp;
        bone.m_Positions.push_back(data);
    }

    bone.m_NumRotations = channel->mNumRotationKeys;
    for (int rotationIndex = 0; rotationIndex < bone.m_NumRotations; ++rotationIndex)
    {
        aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
        float timeStamp = 
            static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
        KeyRotation data{};
        data.orientation.X = aiOrientation.x;
        data.orientation.Y = aiOrientation.y;
        data.orientation.Z = aiOrientation.z;
        data.orientation.W = aiOrientation.w;
        data.timeStamp = timeStamp;
        bone.m_Rotations.push_back(data);
    }

    bone.m_NumScalings = channel->mNumScalingKeys;
    for (int keyIndex = 0; keyIndex < bone.m_NumScalings; ++keyIndex)
    {
        aiVector3D aiScale = channel->mScalingKeys[keyIndex].mValue;
        float timeStamp = 
            static_cast<float>(channel->mScalingKeys[keyIndex].mTime);
        KeyScale data{};
        data.scale.X = aiScale.x;
        data.scale.Y = aiScale.y;
        data.scale.Z = aiScale.z;
        data.timeStamp = timeStamp;
        bone.m_Scales.push_back(data);
    }

    return bone;
}

void AssetImporter::ExtractBoneWeightForVertices(
    std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene, Model& model)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (model.BoneInfoMap.find(boneName) == model.BoneInfoMap.end())
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = model.BoneCounter;
            newBoneInfo.offset = TransposeAndConvertMatrix(
                mesh->mBones[boneIndex]->mOffsetMatrix);
            model.BoneInfoMap[boneName] = newBoneInfo;
            boneID = model.BoneCounter;
            model.BoneCounter++;
        }
        else
        {
            boneID = model.BoneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }
}