#pragma once

#include "math/handmade_math.h"
#include "assets/assets.h"

struct GameMemory
{
    /*

    Permanent memory :

    Allocated once never reset(only cleared on level restart or exit).
    Used for persistent game objects, resources, etc.

    Transient memory :

    Reused every frame.
    Great for pathfinding nodes, temporary meshes, collision queries, etc.
    Prevents constant heap allocations and fragmentation.

    */

    void* PermanentStorage;
    uint64_t PermanentCapacity;

    void* TransientStorage;
    uint64_t TransientCapacity;
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

struct Entity
{
    Model Model{};
    M4 WorldMatrix{};
};

constexpr size_t MAX_ENTITIES = 1024;
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

