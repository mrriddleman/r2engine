#ifndef __ANIMATION_CLIP_H__
#define __ANIMATION_CLIP_H__

#include "r2/Core/Assets/AssetTypes.h"

namespace r2::mem::utils
{
	struct MemoryProperties;
}

namespace flat
{
	struct RAnimation;
}

namespace r2::anim
{
	struct TransformTrack;
	struct Pose;

	struct AnimationClip
	{
		r2::asset::AssetName mAssetName;
		float mStartTime;
		float mEndTime;
		r2::SArray<TransformTrack*>* mTracks;

		float AdjustTimeToFitRange(float inTime, bool loop) const;
		float GetDuration() const;
		u32 GetJointIDAtIndex(u32 index) const;
		u32 Size() const;
		float Sample(Pose& outPose, float inTime, bool loop) const;

		static u64 MemorySize(u32 numTransformTracks, const r2::mem::utils::MemoryProperties& memProperties);
	};

	AnimationClip* LoadAnimationClip(void** memoryPointer, const flat::RAnimation* rAnimation);
}

#endif
