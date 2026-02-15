#pragma once
#include <assimp/scene.h>
#include <assets/assets.h>

class AssetImporter
{
public:
    static void LoadModel(std::string path, Model& model);

private:
    static void ProcessNode(aiNode* node, const aiScene* scene, Model& model);
    static void ProcessMesh(aiMesh* mesh, const aiScene* scene, Model& model);
    static std::vector<Texture> LoadMaterialTextures(
        aiMaterial* mat, 
        aiTextureType type,
        std::string typeName);

}; inline static std::string _Directory;

