#include "r2pch.h"

#include "r2/Render/Animation/AnimationClip.h"
#include "r2/Render/Animation/TransformTrack.h"
#include "r2/Core/Math/MathUtils.h"
#include "r2/Render/Animation/Pose.h"
#include "assetlib/RAnimation_generated.h"

namespace r2::anim
{
	float AnimationClip::AdjustTimeToFitRange(float inTime, bool loop) const
	{
		if (loop)
		{
			float duration = GetDuration();
			if (duration <= 0)
			{
				return 0.0f;
			}

			inTime = fmodf(inTime - mStartTime, duration);
			if (inTime < 0.0f)
			{
				inTime += duration;
			}
			inTime += mStartTime;
		}
		else
		{
			if (inTime < mStartTime)
			{
				inTime = mStartTime;
			}
			if (inTime > mEndTime)
			{
				inTime = mEndTime;
			}
		}

		return inTime;
	}

	float AnimationClip::GetDuration() const
	{
		return mEndTime - mStartTime;
	}

	u32 AnimationClip::GetJointIDAtIndex(u32 index) const
	{
		return mTracks->mData[index]->mJointID;
	}

	u32 AnimationClip::Size() const
	{
		return mTracks->mSize;
	}

	float AnimationClip::Sample(Pose& outPose, float inTime, bool loop) const
	{
		if (math::NearZero(GetDuration()))
		{
			printf("Near Zero\n");
			return 0.0f;
		}

		inTime = AdjustTimeToFitRange(inTime, loop);

		u32 size = mTracks->mSize;
		for (u32 i = 0; i < size; ++i)
		{
			u32 j = mTracks->mData[i]->mJointID;
			math::Transform local = pose::GetLocalTransform(outPose, j);
			math::Transform animated = mTracks->mData[i]->Sample(local, inTime, loop);
			pose::SetLocalTransform(outPose, j, animated);
		}

		return inTime;
	}

	u64 AnimationClip::MemorySize(u32 numTransformTracks, const r2::mem::utils::MemoryProperties& memProperties)
	{
		return
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimationClip), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<TransformTrack*>::MemorySize(numTransformTracks), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
	}

	AnimationClip* LoadAnimationClip(void** memoryPointer, const flat::RAnimation* rAnimation)
	{
		AnimationClip* newAnimationClip = new (*memoryPointer) AnimationClip();
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, sizeof(AnimationClip));

		const auto flatTracks = rAnimation->tracks();
		const auto numTracks = flatTracks->size();

		newAnimationClip->mTracks = EMPLACE_SARRAY(*memoryPointer, TransformTrack*, numTracks);
		*memoryPointer = r2::mem::utils::PointerAdd(*memoryPointer, r2::SArray<TransformTrack*>::MemorySize(numTracks));

		for (flatbuffers::uoffset_t i = 0; i < numTracks; ++i)
		{
			TransformTrack* nextTransformTrack = LoadTransformTrack(memoryPointer, flatTracks->Get(i));
			r2::sarr::Push(*newAnimationClip->mTracks, nextTransformTrack);
		}

		r2::asset::MakeAssetNameFromFlatAssetName(rAnimation->assetName(), newAnimationClip->mAssetName);
		newAnimationClip->mStartTime = rAnimation->startTime();
		newAnimationClip->mEndTime = rAnimation->endTime();

		return newAnimationClip;
	}

}