#include "r2pch.h"

#include "r2/Render/Animation/TransformTrack.h"
#include "assetlib/RAnimation_generated.h"

namespace r2::anim
{

	bool TransformTrack::IsValid()
	{
		return
			(mPosition && mPosition->Size() > 0) ||
			(mScale && mScale->Size() > 0) ||
			(mRotation && mRotation->Size() > 0);
	}

	r2::math::Transform TransformTrack::Sample(const math::Transform& ref, float time, bool looping)
	{
		math::Transform result = ref;

		if (mPosition->Size() > 1)
		{
			result.position = mPosition->Sample(time, looping);
		}

		if (mRotation->Size() > 1)
		{
			result.rotation = mRotation->Sample(time, looping);
		}

		if (mScale->Size() > 1)
		{
			result.scale = mScale->Sample(time, looping);
		}

		return result;
	}

	u64 TransformTrack::MemorySize(const r2::mem::utils::MemoryProperties& memProperties)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TransformTrack), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
	}

	r2::anim::TransformTrack* LoadTransformTrack(void** memoryPointer, const flat::TransformTrack* flatTransformTrack)
	{
		TransformTrack* newTransformTrack = new (*memoryPointer) TransformTrack();
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, sizeof(TransformTrack));

		newTransformTrack->mPosition = LoadVectorTrack(memoryPointer, flatTransformTrack->positionTrack());
		newTransformTrack->mScale = LoadVectorTrack(memoryPointer, flatTransformTrack->scaleTrack());
		newTransformTrack->mRotation = LoadQuatTrack(memoryPointer, flatTransformTrack->rotationTrack());

		newTransformTrack->mJointID = flatTransformTrack->jointID();
		newTransformTrack->mStartTime = flatTransformTrack->startTime();
		newTransformTrack->mEndTime = flatTransformTrack->endTime();

		return newTransformTrack;
	}

}