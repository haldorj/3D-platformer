#pragma once

#include "math/handmade_math.h"

struct Vertex
{
    V3 Position{};
    V3 Normal{};
    V2 TexCoord{};
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
};
