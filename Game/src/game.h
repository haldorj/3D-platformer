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
    int Width{};
    int Height{};
    std::vector<unsigned char> Pixels{};
};

struct DirectionalLight
{
    V4 Color{};
    V4 Direction{};
    V4 Ambient{};
    V4 Diffuse{};
};

struct FontGlyph
{
    void* TextureView{};
    V2 Size{};
    V2 Bearing{};
    float Advance{};
};

struct Camera
{
    M4 View{};
    M4 Projection{};

    V3 Position{};
    V3 Direction{};
    V3 Up{};
};

/*
struct GameMemory
{
    uint64_t PermanentStorageSize;
    uint64_t TransientStorageSize;
    void* PermanentStorage; // Required to be cleared to zero at startup
    void* TransientStorage; // Required to be cleared to zero at startup

    bool IsInitialized;
};
*/

struct GameState
{
    std::unordered_map<char, FontGlyph> LoadedFontGlyphs{};

    Camera MainCamera{};

    DirectionalLight GlobalDirectionalLight{};

    M4 Cube1World;
    M4 Cube2World;

    M4 Rotation;
    M4 Scale;
    M4 Translation;

    float Rot = 0.01f;
};

