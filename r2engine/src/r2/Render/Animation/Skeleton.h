#ifndef __SKELETON_H__
#define __SKELETON_H__

#include <glm/glm.hpp>

namespace r2::mem::utils
{
	struct MemoryProperties;
}

namespace flat
{
	struct AnimationData;
}

namespace r2::anim
{
	struct Pose;

	struct Skeleton
	{
		Pose* mRestPose = nullptr;
		Pose* mBindPose = nullptr;
		r2::SArray<glm::mat4>* mInvBindPose = nullptr;

#ifdef R2_EDITOR
		std::vector<std::string> mJointNames;
#endif

		static u64 MemorySize(u32 numJoints, const r2::mem::utils::MemoryProperties& memProperties);
	};

	Skeleton LoadSkeleton(void** memoryPointer, const flat::AnimationData* animationData);
}

#endif