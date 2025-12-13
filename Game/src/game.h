#pragma once

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

struct CbPerObject
{
    M4 Projection{};
    M4 View{};
    M4 World{};
    V4 Color{};
};

struct CbPerFrame
{
    DirectionalLight Light{};
};

struct FontGlyph
{
    ID3D11ShaderResourceView* TextureView{};
    V2 Size{};
    V2 Bearing{};
    float Advance{};
};