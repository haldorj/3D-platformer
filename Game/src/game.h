#pragma once

#include "handmade_math.h"

class Platform;
class Renderer;

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

struct GameState
{
    DirectionalLight GlobalDirectionalLight{};

    M4 Cube1World;
    M4 Cube2World;
    M4 CamView;
    M4 CamProjection;

    V3 CamPosition;
    V3 CamTarget;
    V3 CamUp;

    M4 Rotation;
    M4 Scale;
    M4 Translation;
    float Rot = 0.01f;

    std::unordered_map<char, FontGlyph> LoadedFontGlyphs{};
};

