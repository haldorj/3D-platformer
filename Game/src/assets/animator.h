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

    UpdateAnimation(animator, animator.CurrentTime);
}

void UpdateAnimation(Animator& animator, float time)
{
    for (auto& channel : animator.CurrentAnimation->Channels)
    {
        int node = channel.TargetNode;
        V3 t = InterpolateVec3(channel.Times, channel.Translations, time);
        Quat r = InterpolateQuat(channel.Times, channel.Rotations, time);
        V3 s = InterpolateVec3(channel.Times, channel.Scales, time);

        animator.FinalBoneTransforms[node] =
            MatrixTranslation(t.X, t.Y, t.Z) *
            MatrixFromQuaternion(r) *
            MatrixScaling(s.X, s.Y, s.Z);
    }

    std::function<void(int, const M4&)> UpdateNodeTransform =
        [&](int boneIndex, const M4& parentTransform)
        {
            BoneInfo& bone = animator.TargetSkeleton->Bones[boneIndex];
            M4 global = parentTransform * animator.FinalBoneTransforms[bone.ID];
            bone.FinalTransform = global * bone.OffsetMatrix;

            for (int child : bone.Children)
                UpdateNodeTransform(child, global);
        };

    if (animator.TargetSkeleton->RootBone >= 0)
        UpdateNodeTransform(animator.TargetSkeleton->RootBone, MatrixIdentity());
}