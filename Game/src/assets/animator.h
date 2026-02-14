#pragma once

struct Animator;
struct Animation;
struct Skeleton;
struct JointAnimation;
struct M4;

class AnimationSystem 
{ 
public:
	static void PlayAnimation(Animator& animator, Animation* animation,
		Skeleton* skeleton, float playbackSpeed = 1.0f, bool looping = false);
	static void UpdateAnimator(Animator& animator, float deltaTime);

private:
	static M4 CalculateJointLocalTransform(
		const JointAnimation& jointAnimation, float currentAnimationTime);
	static void UpdateAnimation(Animator& animator);
};

