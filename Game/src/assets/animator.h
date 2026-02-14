#pragma once

#include <math/handmade_math.h>
#include <assets/assets.h>

void UpdateAnimation(Animator& animator);

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
    UpdateAnimation(animator);
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

M4 CalculateJointLocalTransform(const JointAnimation& jointAnimation, float currentAnimationTime)
{
    V3   t{ 0,0,0 };
    Quat r{ 1,0,0,0 };
    V3   s{ 1,1,1 };

    if (!jointAnimation.Translations.empty())
    {
        auto& currentFrame = jointAnimation.Translations[0];
        for (int i = 1; i < jointAnimation.Translations.size(); ++i)
        {
            auto& nextFrame = jointAnimation.Translations[i];
            if (nextFrame.Time > currentAnimationTime)
            {
                float totalTime = nextFrame.Time - currentFrame.Time;
                float currentTime = currentAnimationTime - currentFrame.Time;
                float progression = currentTime / totalTime;

                t = V3Lerp(currentFrame.Value, nextFrame.Value, progression);
                break;
            }
        }
    }

    if (!jointAnimation.Rotations.empty())
    {
        auto& currentFrame = jointAnimation.Rotations[0];
        for (int i = 1; i < jointAnimation.Rotations.size(); ++i)
        {
            auto& nextFrame = jointAnimation.Rotations[i];
            if (nextFrame.Time > currentAnimationTime)
            {
                float totalTime = nextFrame.Time - currentFrame.Time;
                float currentTime = currentAnimationTime - currentFrame.Time;
                float progression = currentTime / totalTime;

                r = Slerp(currentFrame.Value, nextFrame.Value, progression);
                break;
            }
        }
    }

    if (!jointAnimation.Scales.empty())
    {
        auto& currentFrame = jointAnimation.Scales[0];
        for (int i = 1; i < jointAnimation.Scales.size(); ++i)
        {
            auto& nextFrame = jointAnimation.Scales[i];
            if (nextFrame.Time > currentAnimationTime)
            {
                float totalTime = nextFrame.Time - currentFrame.Time;
                float currentTime = currentAnimationTime - currentFrame.Time;
                float progression = currentTime / totalTime;

                s = V3Lerp(currentFrame.Value, nextFrame.Value, progression);
                break;
            }
        }
    }
    //return MatrixScaling(s) * MatrixFromQuaternion(r) * MatrixTranslation(t);
    return MatrixTranslation(t) * MatrixFromQuaternion(r) * MatrixScaling(s);
}

void UpdateAnimation(Animator& animator)
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

        // 2. Post-multiply the parents transform
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

        joint.GlobalTransform = parentGlobalTransform * joint.LocalTransform;
        animator.FinalBoneTransforms[joint.ID] = joint.GlobalTransform;   
    }

    for (auto& joint : skeleton.Joints)
    {
        animator.FinalBoneTransforms[joint.ID] = 
            animator.FinalBoneTransforms[joint.ID] * joint.InverseBindTransform;
    }
}
