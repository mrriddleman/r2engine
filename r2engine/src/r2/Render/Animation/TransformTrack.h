#ifndef __TRANSFORM_TRACK_H__
#define __TRANSFORM_TRACK_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Math/Transform.h"
#include "r2/Render/Animation/Track.h"

namespace r2::mem::utils
{
	struct MemoryProperties;
}

namespace flat
{
	struct TransformTrack;
}

namespace r2::anim
{
	struct TransformTrack
	{
		u32 mJointID;
		VectorTrack* mPosition = nullptr;
		VectorTrack* mScale = nullptr;
		QuatTrack* mRotation = nullptr;
		float mStartTime;
		float mEndTime;

		bool IsValid();
		math::Transform Sample(const math::Transform& ref, float time, bool looping);

		static u64 MemorySize(const r2::mem::utils::MemoryProperties& memProperties);
	};

	TransformTrack* LoadTransformTrack(void** memoryPointer, const flat::TransformTrack* transformTrack);
}

#endif // __TRANSFORM_TRACK_H__
