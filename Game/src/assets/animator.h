#pragma once

//struct Animator;
//struct Animation;
//struct Skeleton;
//struct JointAnimation;
//struct M4;
#include <assets/assets.h>

class AnimationSystem 
{ 
public:
//	static void PlayAnimation(Animator& animator, Animation* animation,
//		Skeleton* skeleton, float playbackSpeed = 1.0f, bool looping = false);
//	static void UpdateAnimator(Animator& animator, float deltaTime);
//
//private:
//	static M4 CalculateJointLocalTransform(
//		const JointAnimation& jointAnimation, float currentAnimationTime);
//	static void UpdateAnimation(Animator& animator);
	static void UpdateBoneLocalTransform(struct Bone& bone, float animationTime);

private:
	void UpdateAnimation(Animator& animator, float dt);
	void PlayAnimation(Animator& animator, Animation* pAnimation);
	Bone* FindBone(Animation* animation, const std::string& name);
	void CalculateBoneTransform(Animator& animator, const AssimpNodeData* node, M4 parentTransform);
	static M4 InterpolatePosition(Bone& bone, float animationTime);
	static M4 InterpolateRotation(Bone& bone, float animationTime);
	static M4 InterpolateScaling(Bone& bone, float animationTime);
	
	static int GetPositionIndex(Bone& bone, float animationTime);
	static int GetRotationIndex(Bone& bone, float animationTime);
	static int GetScaleIndex(Bone& bone, float animationTime);

	static float GetInterpProgression(float lastTimeStamp, float nextTimeStamp, float animationTime);
	
};

