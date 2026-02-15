#include "pch.h"
#include "asset_importer.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

void AssetImporter::LoadModel(std::string path, Model& model)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::println("ERROR::ASSIMP:: {}", import.GetErrorString());
        return;
    }
    _Directory = path.substr(0, path.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene, model);
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
