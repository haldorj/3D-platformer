#include <pch.h>

#include "animator.h"
#include <assets/assets.h>

void AnimationSystem::PlayAnimation(Animator& animator, Animation* animation,
    Skeleton* skeleton, float playbackSpeed, bool looping)
{
    animator.TargetSkeleton = skeleton;
    animator.CurrentAnimation = animation;

    animator.CurrentTime = 0.0f;
    animator.PlaybackSpeed = playbackSpeed;
    animator.Looping = looping;
}

void AnimationSystem::UpdateAnimator(Animator& animator, float deltaTime)
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
    UpdateAnimation(animator);
}

M4 AnimationSystem::CalculateJointLocalTransform(const JointAnimation& jointAnimation, float currentAnimationTime)
{
    V3 t{ 0,0,0 };
    Quat r{ 1,0,0,0 };
    V3 s{ 1,1,1 };

    // Helper lambda to find interpolated value for each track
    auto interpolateVec3 = [&](const std::vector<KeyframeV3>& frames, V3& out) {
        if (frames.empty()) return;
        if (currentAnimationTime <= frames.front().Time) { out = frames.front().Value; return; }
        if (currentAnimationTime >= frames.back().Time) { out = frames.back().Value;  return; }
        for (size_t i = 0; i + 1 < frames.size(); ++i) {
            if (frames[i + 1].Time >= currentAnimationTime) {
                float total = frames[i + 1].Time - frames[i].Time;
                float local = currentAnimationTime - frames[i].Time;
                float t01 = local / total;
                out = V3Lerp(frames[i].Value, frames[i + 1].Value, t01);
                return;
            }
        }
        };

    auto interpolateQuat = [&](const std::vector<KeyframeQuat>& frames, Quat& out) {
        if (frames.empty()) return;
        if (currentAnimationTime <= frames.front().Time) { out = frames.front().Value; return; }
        if (currentAnimationTime >= frames.back().Time) { out = frames.back().Value;  return; }
        for (size_t i = 0; i + 1 < frames.size(); ++i) {
            if (frames[i + 1].Time >= currentAnimationTime) {
                float total = frames[i + 1].Time - frames[i].Time;
                float local = currentAnimationTime - frames[i].Time;
                float t01 = local / total;
                out = Slerp(frames[i].Value, frames[i + 1].Value, t01);
                return;
            }
        }
        };

    interpolateVec3(jointAnimation.Translations, t);
    interpolateQuat(jointAnimation.Rotations, r);
    interpolateVec3(jointAnimation.Scales, s);

    // Choose TRS ordering based on your math conventions
    //return MatrixScaling(s) * MatrixFromQuaternion(r) * MatrixTranslation(t);
    return MatrixTranslation(t) * MatrixFromQuaternion(r) * MatrixScaling(s);
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
- Calculate the T*R*S model matrix of the current bone (C*B)
- If this bone has a parent, post multiply the parents transform (D)
- IMPORTANT: make sure the first bone we deal with have no children.
2. Go through every bone a second time
- Pre-multiply the inverse bind matrix
T = (DCB) * A

*/

void AnimationSystem::UpdateAnimation(Animator& animator)
{
    if (!animator.CurrentAnimation || !animator.TargetSkeleton)
        return;

    Animation& anim = *animator.CurrentAnimation;
    Skeleton& skeleton = *animator.TargetSkeleton;

    for (auto& joint : skeleton.Joints)
    {
        int targetJointID = joint.ID;
        JointAnimation& targetJointAnim = anim.PerJointAnimationPoses[targetJointID];

        // 1. TRS model matrix
        joint.LocalTransform =
            CalculateJointLocalTransform(targetJointAnim, animator.CurrentTime);
        
        M4 parentGlobalTransform{};
        if (bool hasParent = joint.Parent >= 0 && joint.Parent < (int)skeleton.Joints.size())
        {
            int parentIndex = skeleton.JointIDToArrayIndex[joint.Parent];
            parentGlobalTransform = skeleton.Joints[parentIndex].GlobalTransform;
        }
        else
        {
            parentGlobalTransform = MatrixIdentity();
        }

        // 2. Post-multiply the parents transform
        joint.GlobalTransform = parentGlobalTransform * joint.LocalTransform;
    }

    for (auto& joint : skeleton.Joints)
    {
        animator.FinalBoneTransforms[joint.ID] =
            joint.GlobalTransform * joint.InverseBindTransform;
    }
}
