#ifndef __LEVEL_CACHE_H__
#define __LEVEL_CACHE_H__

#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetFiles/RawAssetFile.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"

namespace flat
{
	struct LevelPackData;
	struct LevelData;
	struct LevelGroupData;
}

namespace r2::mem
{
	namespace utils
	{
		struct MemoryProperties;
	}
}

namespace r2
{
	
	
	struct LevelCache
	{
		r2::mem::utils::MemBoundary mLevelCacheBoundary;
		r2::mem::utils::MemBoundary mAssetBoundary;
		
		r2::mem::StackArena* mArena;
		r2::asset::AssetCache* mLevelCache;

		r2::asset::AssetHandle mLevelPackDataAssetHandle;
		r2::asset::AssetCacheRecord mLevelPackCacheRecord;
		const flat::LevelPackData* mLevelPackData;

		r2::SArray<r2::asset::AssetCacheRecord>* mLevels;

		u32 mMaxNumLevelsInGroup;
		u32 mMaxLevelSizeInBytes;
		u32 mMaxGroupSizeInBytes;
		
	};

	namespace lvlche
	{
		LevelCache* CreateLevelCache(
			const r2::mem::utils::MemBoundary& levelCacheBoundary,
			const char* levelPackDataFilePath,
			const flat::LevelPackData* initialLevelPackData,
			u32 totalNumberOfLevels,
			u32 maxGroupFileSize,
			u32 maxNumLevelsInGroup,
			u32 maxLevelSizeInBytes,
			u32 maxGroupSizeInBytes);
		
		void Shutdown(LevelCache* levelCache);
		u64 MemorySize(u32 maxNumLevels, u32 cacheSizeInBytes, const r2::mem::utils::MemoryProperties& memProperties);

		LevelHandle LoadLevelData(LevelCache& levelCache, const char* levelname);
		LevelHandle LoadLevelData(LevelCache& levelCache, LevelName name);
		LevelHandle LoadLevelData(LevelCache& levelCache, const r2::asset::Asset& levelAsset);

		const flat::LevelData* GetLevelData(LevelCache& levelCache, const LevelHandle& handle);
		void UnloadLevelData(LevelCache& levelCache, const LevelHandle& handle);

		u32 GetNumLevelsInLevelGroup(LevelCache& levelCache, LevelName groupName);
		u32 GetMaxGroupSizeInBytes(LevelCache& levelCache);
		u32 GetMaxLevelSizeInBytes(LevelCache& levelCache);
		u32 GetMaxNumLevelsInAGroup(LevelCache& levelCache);
		u32 GetNumberOfLevelFiles(LevelCache& levelCache);
		u32 GetLevelCapacity(LevelCache& levelCache);

		void LoadLevelGroup(LevelCache& levelCache, LevelName groupName, r2::SArray<LevelHandle>& levelHandles);
		void GetLevelGroupData(LevelCache& levelCache, const r2::SArray<LevelHandle>& levelHandles, r2::SArray<const flat::LevelData*>& levelGroupData);

		void UnloadLevelGroupData(LevelCache& levelCache, const r2::SArray<LevelHandle>& levelHandles);

		void FlushAll(LevelCache& levelCache);

#if defined(R2_ASSET_PIPELINE) && defined(R2_EDITOR)
		void SaveNewLevelFile(LevelCache& levelCache, LevelName group, const char* levelURI, const void* data, u32 dataSize);
#endif

	}
}

#endif
