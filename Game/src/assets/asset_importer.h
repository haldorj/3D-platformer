#pragma once
#include <assimp/scene.h>
#include <assets/assets.h>

class AssetImporter
{
public:
    static void LoadModel(std::string path, Model& model);
    static void LoadAnimation(const std::string& animationPath, Model& model);

private:
    static void ProcessNode(aiNode* node, const aiScene* scene, Model& model);
    static void ProcessMesh(aiMesh* mesh, const aiScene* scene, Model& model);
    static std::vector<Texture> LoadMaterialTextures(
        aiMaterial* mat, 
        aiTextureType type,
        std::string typeName);


    static void ExtractBoneWeightForVertices(
        std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene, Model& model);
    static M4 TransposeAndConvertMatrix(const aiMatrix4x4& from);

    static Bone LoadBone(const std::string& name, int ID, const aiNodeAnim* channel);

    static void ReadMissingBones(const aiAnimation* animation, Model& model, Animation& anim);
    static void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);

}; inline static std::string _Directory;

