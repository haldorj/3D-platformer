#pragma once

#include "math/handmade_math.h"
#include "assets/assets.h"

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

constexpr size_t MAX_ENTITIES = 128;
struct GameWorld
{
	std::array<Entity, MAX_ENTITIES> Entities{};
    DirectionalLight DirectionalLight{};
};

struct GameMemory
{
	GameWorld World{};
    Camera MainCamera{};
};

