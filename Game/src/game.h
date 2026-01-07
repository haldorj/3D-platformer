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

    float Pitch{};
    float Yaw{};
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

struct Entity
{
    Model Model{};
    M4 WorldMatrix{};
};

constexpr size_t MAX_ENTITIES = 12;

struct GameWorld
{
	std::array<Entity, MAX_ENTITIES> Entities{};
};

struct GameState
{
	GameWorld World{};

    Camera MainCamera{};

    std::unordered_map<char, FontGlyph> LoadedFontGlyphs{};
    DirectionalLight GlobalDirectionalLight{};

    float Rot = 0.01f;
};

