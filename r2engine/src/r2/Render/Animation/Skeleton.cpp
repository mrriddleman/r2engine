#include "r2pch.h"
#include "r2/Render/Animation/Skeleton.h"
#include "r2/Render/Animation/Pose.h"
#include "assetlib/RModel_generated.h"
#include "r2/Core/Math/MathUtils.h"

namespace r2::anim
{
	u64 Skeleton::MemorySize(u32 numJoints, const r2::mem::utils::MemoryProperties& memProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Skeleton), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(numJoints), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			static_cast<u64>(Pose::MemorySize(numJoints, memProperties)) * 2; //*2 for rest & bind poses
	}

	r2::anim::Skeleton* LoadSkeleton(void** memoryPointer, const flat::AnimationData* animationData)
	{
		u32 numJoints = animationData->boneInfo()->size();
		Skeleton* skeleton = new (*memoryPointer) Skeleton();
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, sizeof(Skeleton));

		skeleton->mRestPose = pose::Load(memoryPointer, animationData->restPoseTransforms(), animationData->parents());
		skeleton->mBindPose = pose::Load(memoryPointer, animationData->bindPoseTransforms(), animationData->parents());

		const auto boneInfo = animationData->boneInfo();

		skeleton->mInvBindPose = EMPLACE_SARRAY(*memoryPointer, glm::mat4, boneInfo->size());
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray<glm::mat4>::MemorySize(numJoints));

		for (u32 i = 0; i < numJoints; ++i)
		{
			r2::sarr::Push(*skeleton->mInvBindPose, r2::math::GetGLMMatrix4FromFlatMatrix(&boneInfo->Get(i)->offsetTransform()));
		}

#ifdef R2_EDITOR
		for (u32 i = 0; i < animationData->jointNameStrings()->size(); ++i)
		{
			skeleton->mJointNames.push_back(animationData->jointNameStrings()->Get(i)->str());
		}
#endif

		return skeleton;
	}

}