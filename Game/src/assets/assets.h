#pragma once

#include "math/handmade_math.h"

// Maximum number of bones that can influence one vertex.
static constexpr int MAX_BONE_INFLUENCE = 4;
struct Vertex
{
    V3 Position{};
    V3 Normal{};
    V2 TexCoord{};

    std::array<int32_t, MAX_BONE_INFLUENCE> BoneIDs{};
    std::array<float, MAX_BONE_INFLUENCE> Weights{};
};

struct BoneInfo
{
    M4 Matrix;
    int32_t ID;
};

struct Texture
{
    std::vector<unsigned char> Pixels{};
    int Width{};
    int Height{};
};

struct Mesh
{
    std::vector<Texture> Textures{};
    std::vector<Vertex> Vertices{};
    std::vector<uint32_t> Indices{};

    std::vector<void*> TextureViews{};
    void* VertexBuffer{};
    void* IndexBuffer{};
};

struct Model
{
    std::vector<Mesh> Meshes{};
    // std::unordered_map<std::string, BoneInfo> BoneInfoMap{};
};
