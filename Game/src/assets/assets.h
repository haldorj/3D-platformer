#pragma once

#include "math/handmade_math.h"

// Maximum number of bones that can influence one vertex.
static constexpr int MAX_BONE_INFLUENCE = 4;
struct Vertex
{
    V3 Position{};
    V3 Normal{};
    V2 TexCoord{};

    // The bones that can affect this vertex.
    IV4 BoneIDs{};
    // How much influence each bone has on this vertex.
    V4 Weights{};

    // Example:
    // BoneIDs = [6,    2,    ... ]
    // Weights = [0.25, 0.75, ... ] (sum should always be 1).
};

// Used for different primitives (line, points, cubes etc...) 
// for visual debugging.
struct DebugVertex
{
    V3 Position{};
    V3 Color{};
};

struct Joint
{
    // Inverted model-space bind transform (bone to model origin).
    M4 InverseBindTransform{};
    M4 GlobalTransform{};
    M4 LocalTransform{};

    std::string Name{};
    int32_t ID{};
    int32_t Parent{};
};

struct Skeleton 
{
    std::unordered_map<int, int> JointIDToArrayIndex;
    std::vector<Joint> Joints;
};

struct KeyframeV3 
{
    float Time;
    V3 Value;
};

struct KeyframeQuat 
{
    float Time;
    Quat Value;
};

struct JointAnimation 
{
    std::vector<KeyframeV3> Translations;
    std::vector<KeyframeQuat> Rotations;
    std::vector<KeyframeV3> Scales;
    std::string TargetNode;
};

struct Animation
{
    std::string Name;
    std::unordered_map<int, JointAnimation> PerJointAnimationPoses;
    float Duration;
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
