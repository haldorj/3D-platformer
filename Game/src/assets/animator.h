#pragma once

#include <math/handmade_math.h>
#include <assets/assets.h>

void UpdateAnimation(Animator& animator, float time);

static V3 InterpolateVec3(const std::span<float> times, const std::span<V3> values, float time)
{
    if (times.empty() || values.empty())
        return {};

    if (time <= times.front()) return values.front();
    if (time >= times.back())  return values.back();

    for (size_t i = 0; i < times.size() - 1; ++i)
    {
        if (time < times[i + 1])
        {
            float t = (time - times[i]) / (times[i + 1] - times[i]);
            return V3Lerp(values[i], values[i + 1], t);
        }
    }
    return values.back();
}

static Quat InterpolateQuat(const std::span<float> times, const std::span<Quat> values, float time)
{
    if (times.empty() || values.empty())
        return {};

    if (time <= times.front()) return values.front();
    if (time >= times.back())  return values.back();

    for (size_t i = 0; i < times.size() - 1; ++i)
    {
        if (time < times[i + 1])
        {
            float t = (time - times[i]) / (times[i + 1] - times[i]);
            return Slerp(values[i], values[i + 1], t);
        }
    }
    return values.back();
}

static void PlayAnimation(Animator& animator, Animation* animation,
    Skeleton* skeleton, float playbackSpeed = 1.0f, bool looping = false)
{
    animator.TargetSkeleton = skeleton;
    animator.CurrentAnimation = animation;

    animator.CurrentTime = 0.0f;
    animator.PlaybackSpeed = playbackSpeed;
    animator.Looping = looping;
}

static void UpdateAnimator(Animator& animator, float deltaTime)
{
    if (!animator.CurrentAnimation || !animator.TargetSkeleton)
        return;

    animator.CurrentTime += deltaTime * animator.PlaybackSpeed;

    if (animator.Looping)
    {
        if (animator.CurrentTime > animator.CurrentAnimation->Duration)
            animator.CurrentTime = fmod(animator.CurrentTime, animator.CurrentAnimation->Duration);
    }
    else
    {
        animator.CurrentTime = 
            std::min<float>(animator.CurrentTime, animator.CurrentAnimation->Duration);
    }
    // std::println("current time: {}", animator.CurrentTime);
    UpdateAnimation(animator, animator.CurrentTime);
}

/*

Multiplication order for skeletal animation
Model space -> Animation pose -> World Space

Matrix multiplication happens from right to left
scale * translation * rotation; <---
We are able to pre and post multiply a matrix with another matrix

Calculating the animation pose
A: inverse bind matrix
B: rotation/scale
C: translation
D: Parent's Transform
T = D * C * B * A

CPU Skinning
1. Go through every bone
- Calculate the T*R*S model matrix for this animation frame (C*B)
- If this bone has a parent, post multiply the parents transform (D)
- IMPORTANT: make sure the first bone we deal with have no children.
2. Go through every bone a second time
- Pre-multiply the inverse bind matrix
T = (DCB) * A

*/



void UpdateAnimation(Animator& animator, float time)
{
    if (!animator.CurrentAnimation || !animator.TargetSkeleton)
        return;

    auto& anim = *animator.CurrentAnimation;
    auto& skeleton = *animator.TargetSkeleton;

    for (auto& joint : skeleton.Joints)
    {
        int targetID = joint.ID;

        V3   t{ 0,0,0 };
        Quat r{ 1,0,0,0 };
        V3   s{ 1,1,1 };

        for (auto& channel : anim.Channels)
        {
            if (targetID != channel.TargetNode)
                continue;

            if (!channel.Translations.empty()) 
                t = InterpolateVec3(channel.Times, channel.Translations, time);
            if (!channel.Rotations.empty())    
                r = InterpolateQuat(channel.Times, channel.Rotations, time);
            if (!channel.Scales.empty())       
                s = InterpolateVec3(channel.Times, channel.Scales, time);

            joint.LocalTransform =
                MatrixTranslation(t.X, t.Y, t.Z) *
                MatrixFromQuaternion(r) *
                MatrixScaling(s.X, s.Y, s.Z);

            break;
        }
    }

    for (auto& joint : skeleton.Joints)
    {
        M4 ParentTransform{};
        if (bool hasParent = joint.Parent >= 0 && joint.Parent < (int)skeleton.Joints.size())
        {
            ParentTransform = skeleton.Joints[joint.Parent].LocalTransform;
        }
        else
        {
            ParentTransform = MatrixIdentity();
        }

        animator.FinalBoneTransforms[joint.ID] =
            ParentTransform * joint.LocalTransform * joint.InverseBindTransform;
    }
}
