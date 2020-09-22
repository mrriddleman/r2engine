#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "r2/Core/Containers/SArray.h"
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

	struct AnimationChannel
	{
		//std::string name;
		u64 hashName;
		//u32 numPositionKeys = 0;
		//u32 numRotationKeys = 0;
		//u32 numScalingKeys = 0;
		r2::SArray<VectorKey>* positionKeys = nullptr;
		r2::SArray<VectorKey>* scaleKeys = nullptr;
		r2::SArray<RotationKey>* rotationKeys = nullptr;

		static u64 MemorySizeNoData(u64 numPositionKeys, u64 numScaleKeys, u64 numRotationKeys, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

	struct Animation
	{
		
		u64 hashName;
		
		double duration = 0; //in ticks
		double ticksPerSeconds = 0;
		r2::SArray<AnimationChannel>* channels;

		static u64 MemorySizeNoData(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking);
	};
}


#endif // 
