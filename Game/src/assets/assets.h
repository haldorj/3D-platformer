#pragma once

#include "math/handmade_math.h"

// Maximum number of bones that can influence one vertex.
static constexpr int MAX_BONE_INFLUENCE = 4;
struct Vertex
{
    V3 Position{};
    V3 Normal{};
    V2 TexCoord{};

    IV4 BoneIDs{};
    V4 Weights{};
};

struct BoneInfo
{
    M4 InverseBindMatrix{};
    M4 FinalTransform{};
    // Bones connected to this bone.
    std::vector<int> Children{};

    int32_t ID{};
};

struct Skeleton 
{
    std::vector<BoneInfo> Bones;
    std::unordered_map<int, int> NodeToBoneIndex;
    int32_t RootBone{-1};
};

struct AnimationChannel 
{
    std::string Path{};
    std::vector<float> Times{};
    std::vector<V3> Translations{};
    std::vector<Quat> Rotations{};
    std::vector<V3> Scales{};
    int32_t TargetNode{};
};

struct Animation 
{
    std::string Name{};
    std::vector<AnimationChannel> Channels{};
    float Duration{};
    float CurrentTime{};
};

struct Animator
{
    std::array<M4, 100> FinalBoneTransforms{};
    Skeleton* TargetSkeleton{};
    Animation* CurrentAnimation{};

    float CurrentTime{};
    float PlaybackSpeed{1.0f};
    bool Looping{true};
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
    std::vector<Skeleton> Skeletons{};
    std::vector<Animation> Animations{};
    Animator Animator{};
};
