#include <pch.h>

#include "animator.h"
#include <math/handmade_math.h>
#include <assets/assets.h>
//
//void AnimationSystem::PlayAnimation(Animator& animator, Animation* animation,
//    Skeleton* skeleton, float playbackSpeed, bool looping)
//{
//    animator.TargetSkeleton = skeleton;
//    animator.CurrentAnimation = animation;
//
//    animator.CurrentTime = 0.0f;
//    animator.PlaybackSpeed = playbackSpeed;
//    animator.Looping = looping;
//}
//
//void AnimationSystem::UpdateAnimator(Animator& animator, float deltaTime)
//{
//    if (!animator.CurrentAnimation || !animator.TargetSkeleton)
//        return;
//
//    animator.CurrentTime += deltaTime * animator.PlaybackSpeed;
//
//    if (animator.Looping)
//    {
//        if (animator.CurrentTime > animator.CurrentAnimation->Duration)
//            animator.CurrentTime = fmod(animator.CurrentTime, animator.CurrentAnimation->Duration);
//    }
//    else
//    {
//        animator.CurrentTime =
//            std::min<float>(animator.CurrentTime, animator.CurrentAnimation->Duration);
//    }
//    // std::println("current time: {}", animator.CurrentTime);
//    UpdateAnimation(animator);
//}
//
//M4 AnimationSystem::CalculateJointLocalTransform(const JointAnimation& jointAnimation, float currentAnimationTime)
//{
//    V3 t{ 0,0,0 };
//    Quat r{ 1,0,0,0 };
//    V3 s{ 1,1,1 };
//
//    // Helper lambda to find interpolated value for each track
//    auto interpolateVec3 = [&](const std::vector<KeyframeV3>& frames, V3& out) {
//        if (frames.empty()) return;
//        if (currentAnimationTime <= frames.front().Time) { out = frames.front().Value; return; }
//        if (currentAnimationTime >= frames.back().Time) { out = frames.back().Value;  return; }
//        for (size_t i = 0; i + 1 < frames.size(); ++i) {
//            if (frames[i + 1].Time >= currentAnimationTime) {
//                float total = frames[i + 1].Time - frames[i].Time;
//                float local = currentAnimationTime - frames[i].Time;
//                float t01 = local / total;
//                out = V3Lerp(frames[i].Value, frames[i + 1].Value, t01);
//                return;
//            }
//        }
//        };
//
//    auto interpolateQuat = [&](const std::vector<KeyframeQuat>& frames, Quat& out) {
//        if (frames.empty()) return;
//        if (currentAnimationTime <= frames.front().Time) { out = frames.front().Value; return; }
//        if (currentAnimationTime >= frames.back().Time) { out = frames.back().Value;  return; }
//        for (size_t i = 0; i + 1 < frames.size(); ++i) {
//            if (frames[i + 1].Time >= currentAnimationTime) {
//                float total = frames[i + 1].Time - frames[i].Time;
//                float local = currentAnimationTime - frames[i].Time;
//                float t01 = local / total;
//                out = Slerp(frames[i].Value, frames[i + 1].Value, t01);
//                return;
//            }
//        }
//        };
//
//    interpolateVec3(jointAnimation.Translations, t);
//    interpolateQuat(jointAnimation.Rotations, r);
//    interpolateVec3(jointAnimation.Scales, s);
//
//    // Choose TRS ordering based on your math conventions
//    //return MatrixScaling(s) * MatrixFromQuaternion(r) * MatrixTranslation(t);
//    return MatrixTranslation(t) * MatrixFromQuaternion(r) * MatrixScaling(s);
//}
//
//
///*
//
//Multiplication order for skeletal animation
//Model space -> Animation pose -> World Space
//
//Matrix multiplication happens from right to left
//scale * translation * rotation; <---
//We are able to pre and post multiply a matrix with another matrix
//
//Calculating the animation pose
//A: inverse bind matrix
//B: rotation/scale
//C: translation
//D: Parent's Transform
//T = D * C * B * A
//
//CPU Skinning
//1. Go through every bone
//- Calculate the T*R*S model matrix of the current bone (C*B)
//- If this bone has a parent, post multiply the parents transform (D)
//- IMPORTANT: make sure the first bone we deal with have no children.
//2. Go through every bone a second time
//- Pre-multiply the inverse bind matrix
//T = (DCB) * A
//
//*/
//
//void AnimationSystem::UpdateAnimation(Animator& animator)
//{
//    if (!animator.CurrentAnimation || !animator.TargetSkeleton)
//        return;
//
//    Animation& anim = *animator.CurrentAnimation;
//    Skeleton& skeleton = *animator.TargetSkeleton;
//
//    for (auto& joint : skeleton.Joints)
//    {
//        int targetJointID = joint.ID;
//        JointAnimation& targetJointAnim = anim.PerJointAnimationPoses[targetJointID];
//
//        // 1. TRS model matrix
//        joint.LocalTransform =
//            CalculateJointLocalTransform(targetJointAnim, animator.CurrentTime);
//        
//        M4 parentGlobalTransform{};
//        if (bool hasParent = joint.Parent >= 0 && joint.Parent < (int)skeleton.Joints.size())
//        {
//            int parentIndex = skeleton.JointIDToArrayIndex[joint.Parent];
//            parentGlobalTransform = skeleton.Joints[parentIndex].GlobalTransform;
//        }
//        else
//        {
//            parentGlobalTransform = MatrixIdentity();
//        }
//
//        // 2. Post-multiply the parents transform
//        joint.GlobalTransform = parentGlobalTransform * joint.LocalTransform;
//    }
//
//    for (auto& joint : skeleton.Joints)
//    {
//        animator.FinalBoneTransforms[joint.ID] =
//            joint.GlobalTransform * joint.InverseBindTransform;
//    }
//}

void AnimationSystem::UpdateAnimation(Animator& animator, float dt)
{
    animator.DeltaTime = dt;
    if (animator.CurrentAnimation)
    {
        animator.CurrentTime += animator.CurrentAnimation->m_TicksPerSecond * dt;
        animator.CurrentTime = fmod(animator.CurrentTime, animator.CurrentAnimation->m_Duration);
        CalculateBoneTransform(animator, &animator.CurrentAnimation->m_RootNode, MatrixIdentity());
    }
}

void AnimationSystem::PlayAnimation(Animator& animator, Animation* pAnimation)
{
    animator.CurrentAnimation = pAnimation;
    animator.CurrentTime = 0.0f;
}

Bone* AnimationSystem::FindBone(Animation* animation, const std::string& name)
{
    auto iter = std::find_if(animation->m_Bones.begin(), animation->m_Bones.end(),
        [&](const Bone& Bone)
        {
            return Bone.m_Name == name;
        }
    );
    if (iter == animation->m_Bones.end()) return nullptr;
    else return &(*iter);
}

void AnimationSystem::CalculateBoneTransform(Animator& animator, const AssimpNodeData* node, M4 parentTransform)
{
    std::string nodeName = node->name;
    M4 nodeTransform = node->transformation;

    if (!animator.CurrentAnimation)
        return;

    auto& currentAnim = animator.CurrentAnimation;

    Bone* bone = FindBone(currentAnim, nodeName);

    if (bone)
    {
        UpdateBoneLocalTransform(*bone, animator.CurrentTime);
        nodeTransform = bone->m_LocalTransform;
    }

    M4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = animator.CurrentAnimation->m_BoneInfoMap;
    if (boneInfoMap.find(nodeName) != boneInfoMap.end())
    {
        int index = boneInfoMap[nodeName].id;
        M4 offset = boneInfoMap[nodeName].offset;
        animator.FinalBoneTransforms[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(animator, &node->children[i], globalTransformation);
}

/*figures out which position keys to interpolate b/w and performs the interpolation
and returns the translation matrix*/
M4 AnimationSystem::InterpolatePosition(Bone& bone, float animationTime)
{
    if (1 == bone.m_NumPositions)
        return MatrixTranslation(bone.m_Positions[0].position);

    int p0Index = GetPositionIndex(bone, animationTime);
    int p1Index = p0Index + 1;

    float scaleFactor = GetInterpProgression(
        bone.m_Positions[p0Index].timeStamp,
        bone.m_Positions[p1Index].timeStamp, 
        animationTime);

    V3 finalPosition = V3Lerp(
        bone.m_Positions[p0Index].position,
        bone.m_Positions[p1Index].position, 
        scaleFactor);

    return MatrixTranslation(finalPosition);
}

/*figures out which rotations keys to interpolate b/w and performs the interpolation
and returns the rotation matrix*/
M4 AnimationSystem::InterpolateRotation(Bone& bone, float animationTime)
{
    if (1 == bone.m_NumRotations)
    {
        auto rotation = NormalizeQuat(bone.m_Rotations[0].orientation);
        return MatrixFromQuaternion(rotation);
    }

    int p0Index = GetRotationIndex(bone, animationTime);
    int p1Index = p0Index + 1;

    float scaleFactor = GetInterpProgression(
        bone.m_Rotations[p0Index].timeStamp,
        bone.m_Rotations[p1Index].timeStamp, 
        animationTime);

    Quat finalRotation = 
        Slerp(bone.m_Rotations[p0Index].orientation,
        bone.m_Rotations[p1Index].orientation, 
            scaleFactor);

    finalRotation = NormalizeQuat(finalRotation);
    return MatrixFromQuaternion(finalRotation);
}

/*figures out which scaling keys to interpolate b/w and performs the interpolation
and returns the scale matrix*/
M4 AnimationSystem::InterpolateScaling(Bone& bone, float animationTime)
{
    if (1 == bone.m_NumScalings)
        return MatrixScaling(bone.m_Scales[0].scale);

    int p0Index = GetScaleIndex(bone, animationTime);
    int p1Index = p0Index + 1;

    float scaleFactor = GetInterpProgression(
        bone.m_Scales[p0Index].timeStamp,
        bone.m_Scales[p1Index].timeStamp, 
        animationTime);

    V3 finalScale = V3Lerp(
        bone.m_Scales[p0Index].scale, 
        bone.m_Scales[p1Index].scale,
        scaleFactor);

    return MatrixScaling(finalScale);
}

void AnimationSystem::UpdateBoneLocalTransform(Bone& bone, float animationTime)
{
    M4 translation = InterpolatePosition(bone, animationTime);
    M4 rotation = InterpolateRotation(bone, animationTime);
    M4 scale = InterpolateScaling(bone, animationTime);
    bone.m_LocalTransform = translation * rotation * scale;
}

/* Gets the current index on mKeyPositions to interpolate to based on
the current animation time*/
int AnimationSystem::GetPositionIndex(Bone& bone, float animationTime)
{
    for (int index = 0; index < bone.m_NumPositions - 1; ++index)
    {
        if (animationTime < bone.m_Positions[index + 1].timeStamp)
            return index;
    }
    assert(0);
    return -1;
}

/* Gets the current index on mKeyRotations to interpolate to based on the
current animation time*/
int AnimationSystem::GetRotationIndex(Bone& bone, float animationTime)
{
    for (int index = 0; index < bone.m_NumRotations - 1; ++index)
    {
        if (animationTime < bone.m_Rotations[index + 1].timeStamp)
            return index;
    }
    assert(0);
    return -1;
}

/* Gets the current index on mKeyScalings to interpolate to based on the
current animation time */
int AnimationSystem::GetScaleIndex(Bone& bone, float animationTime)
{
    for (int index = 0; index < bone.m_NumScalings - 1; ++index)
    {
        if (animationTime < bone.m_Scales[index + 1].timeStamp)
            return index;
    }
    assert(0);
    return -1;
}

/* Gets normalized value for Lerp & Slerp*/
float AnimationSystem::GetInterpProgression(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
    float scaleFactor = 0.0f;
    float midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}
