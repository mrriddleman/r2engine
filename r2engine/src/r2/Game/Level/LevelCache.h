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

		r2::SArray<r2::asset::AssetCacheRecord>* mLevels;	

		u32 mLevelCapacity;
	};

	namespace lvlche
	{
		//@TODO(Serge): we should change the level directory out of the levelPackData file
		LevelCache* CreateLevelCache(const r2::mem::utils::MemBoundary& levelCacheBoundary, const char* levelDirectory, u32 levelCapacity, u32 cacheSize);
		
		void Shutdown(LevelCache* levelCache);
		u64 MemorySize(u32 maxNumLevels, u32 cacheSizeInBytes, const r2::mem::utils::MemoryProperties& memProperties);

		LevelHandle LoadLevelData(LevelCache& levelCache, const char* levelname);
		LevelHandle LoadLevelData(LevelCache& levelCache, LevelName name);
		LevelHandle LoadLevelData(LevelCache& levelCache, const r2::asset::Asset& levelAsset);

		const flat::LevelData* GetLevelData(LevelCache& levelCache, const LevelHandle& handle);
		void UnloadLevelData(LevelCache& levelCache, const LevelHandle& handle);

		u32 GetLevelCapacity(LevelCache& levelCache);

		bool HasLevel(LevelCache& levelCache, const char* levelURI);
		bool HasLevel(LevelCache& levelCache, const LevelName name);
		bool HasLevel(LevelCache& levelCache, const r2::asset::Asset& levelAsset);

		void FlushAll(LevelCache& levelCache);
#if defined(R2_ASSET_PIPELINE) && defined(R2_EDITOR)
		bool SaveNewLevelFile(LevelCache& levelCache, const char* levelPath, const void* data, u32 dataSize);
#endif
	}
}

#endif
