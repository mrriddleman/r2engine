#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace r2::draw
{
	struct VectorKey
	{
		double time;
		glm::vec3 value;
	};

	struct RotationKey
	{
		double time;
		glm::quat quat;
	};

	struct ChannelState
	{
		f64 mCurScaleTime = 0.0;
		f64 mCurRotationTime = 0.0;
		f64 mCurTranslationTime = 0.0;

		u32 mCurScaleIndex = 0;
		u32 mCurRotationIndex = 0;
		u32 mCurTranslationIndex = 0;
	};

	struct AnimationChannel
	{
		u64 hashName = 0;
		ChannelState state;

		r2::SArray<VectorKey>* positionKeys = nullptr;
		r2::SArray<VectorKey>* scaleKeys = nullptr;
		r2::SArray<RotationKey>* rotationKeys = nullptr;

		static u64 MemorySizeNoData(u64 numPositionKeys, u64 numScaleKeys, u64 numRotationKeys, float duration, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	struct Animation
	{
		u64 assetName;
		
		double duration = 0; //in ticks
		double ticksPerSeconds = 0;
		r2::SHashMap<AnimationChannel>* channels;

		static u64 MemorySizeNoData(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking);
		static u64 MemorySize(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking);
	};
}


#endif // 
