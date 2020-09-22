#include "r2pch.h"
#include "r2/Render/Animation/Animation.h"

namespace r2::draw
{
	u64 AnimationChannel::MemorySizeNoData(u64 numPositionKeys, u64 numScaleKeys, u64 numRotationKeys, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VectorKey>::MemorySize(numPositionKeys), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VectorKey>::MemorySize(numScaleKeys), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RotationKey>::MemorySize(numRotationKeys), alignment, headerSize, boundsChecking);
	}

	u64 Animation::MemorySizeNoData(u64 numChannels, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<AnimationChannel>::MemorySize(numChannels), alignment, headerSize, boundsChecking);
	}
}