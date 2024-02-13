#include "r2pch.h"

#include "r2/Game/Level/LevelRenderSettings.h"

namespace r2
{
	const u32 LevelRenderSettings::MAX_NUM_LEVEL_CAMERAS = 10;

	u32 LevelRenderSettings::MemorySize(const r2::mem::utils::MemoryProperties& memProperties)
	{
		u32 totalMemorySize = 0;
		totalMemorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(LevelRenderSettings), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		totalMemorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Camera*>::MemorySize(MAX_NUM_LEVEL_CAMERAS), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);

		return totalMemorySize;
	}
}