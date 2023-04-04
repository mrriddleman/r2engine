
#include "r2pch.h"
#include "r2/Game/Level/LevelCache.h"

namespace r2::lvlche
{
	LevelCache* Init(const r2::mem::utils::MemBoundary& levelCacheBoundary, b32 cacheLevelReferences, const r2::asset::RawAssetFile* levelPackFile)
	{
		return nullptr;
	}

	void Shutdown(LevelCache* levelCache)
	{

	}

	u64 MemorySize(u32 maxNumLevels, u32 cacheSizeInBytes, const r2::mem::utils::MemoryProperties& memProperties)
	{
		return 0;
	}

	LevelHandle LoadLevelData(LevelCache& levelCache, const char* levelname)
	{
		return {};
	}

	LevelHandle LoadLevelData(LevelCache& levelCache, LevelName name)
	{
		return {  };
	}

	LevelHandle LoadLevelData(LevelCache& levelCache, const r2::asset::Asset& levelAsset)
	{
		return {};
	}

	flat::LevelData* GetLevelData(LevelCache& levelCache, const LevelHandle& handle)
	{
		return nullptr;
	}

	void UnloadLevelData(LevelCache& levelCache, const LevelHandle& handle)
	{

	}

	u32 GetNumLevelsInLevelGroup(LevelCache& levelCache, LevelName groupName)
	{
		return 0;
	}

	u32 GetMaxLevelSizeInBytes(LevelCache& levelCache)
	{
		return 0;
	}

	u32 GetMaxNumLevelsInAGroup(LevelCache& levelCache)
	{
		return 0;
	}

	void LoadLevelGroup(LevelCache& levelCache, LevelName groupName, r2::SArray<LevelHandle>& levelHandles)
	{

	}

	void GetLevelGroupData(LevelCache& levelCache, const r2::SArray<LevelHandle>& levelHandles, r2::SArray<flat::LevelGroupData*>& levelGroupData)
	{

	}

	void UnloadLevelGroupData(LevelCache& levelCache, const r2::SArray<LevelHandle>& levelHandles)
	{

	}

	void FlushAll(LevelCache& levelCache)
	{

	}

#if defined R2_ASSET_PIPELINE && R2_EDITOR
	void SaveNewLevelFile(LevelCache& levelCache, LevelName group, const char* levelURI, flat::LevelData* levelData)
	{

	}
#endif
}