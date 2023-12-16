#include "r2pch.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"
#include "r2/Utils/Hash.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"



namespace r2
{
	
	GameAssetManager::GameAssetManager()
		:mAssetCache(nullptr)
		,mCachedRecords(nullptr)

	{
	}

	GameAssetManager::~GameAssetManager()
	{
		mAssetCache = nullptr;
		mCachedRecords = nullptr;
	}

	u64 GameAssetManager::MemorySizeForGameAssetManager(u32 numFiles, u32 alignment, u32 headerSize)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		u64 memorySize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(GameAssetManager), alignment, headerSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetCacheRecord>::MemorySize(numFiles), alignment, headerSize, boundsChecking);

		return memorySize;
	}

	u64 GameAssetManager::CacheMemorySize(u32 numAssets, u32 assetCapacity, u32 alignment, u32 headerSize, u32 boundsChecking, u32 lruCapacity, u32 mapCapacity)
	{
		u64 memorySize = r2::asset::AssetCache::TotalMemoryNeeded(numAssets, assetCapacity, alignment, lruCapacity, mapCapacity);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::asset::AssetCache), alignment, headerSize, boundsChecking);
		return memorySize;
	}

	u64 GameAssetManager::GetAssetDataSize(r2::asset::AssetHandle assetHandle)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet!");
			return 0;
		}

		r2::asset::AssetCacheRecord result = FindAssetCacheRecord(assetHandle);

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
		{
			return result.GetAssetBuffer()->Size();
		}

		result = mAssetCache->GetAssetBuffer(assetHandle);

		R2_CHECK(result.GetAssetBuffer()->IsLoaded(), "Not loaded?");

		//store the record
		r2::sarr::Push(*mCachedRecords, result);


		return result.GetAssetBuffer()->Size();
	}

	r2::asset::AssetHandle GameAssetManager::LoadAsset(const r2::asset::Asset& asset)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return {};
		}

		return mAssetCache->LoadAsset(asset);
	}

	void GameAssetManager::UnloadAsset(const u64 assethandle)
	{
		if (!mAssetCache)
			return;

		UnloadAsset({ assethandle, mAssetCache->GetSlot() });
	}

	void GameAssetManager::UnloadAsset(const r2::asset::AssetName& assetName)
	{
		if (!mAssetCache)
			return;

		UnloadAsset({ assetName.hashID, mAssetCache->GetSlot() });
	}

	void GameAssetManager::UnloadAsset(const r2::asset::AssetHandle& assetHandle)
	{
		if (!mAssetCache || !mCachedRecords)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		r2::asset::AssetCacheRecord defaultAssetCacheRecord;
		r2::asset::AssetCacheRecord result = FindAssetCacheRecord(assetHandle);

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
		{
			RemoveAssetCacheRecord(assetHandle);

			bool wasReturned = mAssetCache->ReturnAssetBuffer(result);
			
			if (!wasReturned)
			{
				R2_CHECK(wasReturned, "Somehow we couldn't return the asset cache record");
			}
			
		}

		mAssetCache->FreeAsset(assetHandle);
	}

	r2::asset::AssetHandle GameAssetManager::ReloadAsset(const r2::asset::Asset& asset)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return {};
		}

		r2::asset::AssetCacheRecord defaultAssetCacheRecord;
		r2::asset::AssetCacheRecord result = FindAssetCacheRecord({ asset.HashID(), mAssetCache->GetSlot() });

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
		{

			RemoveAssetCacheRecord({ asset.HashID(), mAssetCache->GetSlot() });

			bool wasReturned = mAssetCache->ReturnAssetBuffer(result);

			R2_CHECK(wasReturned, "Somehow we couldn't return the asset cache record");
		}

		return mAssetCache->ReloadAsset(asset);
	}

	void GameAssetManager::RegisterAssetLoader(r2::asset::AssetLoader* assetLoader)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		mAssetCache->RegisterAssetLoader(assetLoader);
	}

	

	s64 GameAssetManager::GetAssetCacheSlot() const
	{
		if (!mAssetCache)
		{
			return -1;
		}
		return mAssetCache->GetSlot();
	}

	void GameAssetManager::FreeAllAssets()
	{
		if (!mAssetCache || !mCachedRecords)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		const auto numCacheRecords = r2::sarr::Size(*mCachedRecords);

		for (u32 i = 0; i < numCacheRecords; ++i)
		{
			const r2::asset::AssetCacheRecord& record = r2::sarr::At(*mCachedRecords, i);

			if (!record.IsEmptyAssetCacheRecord(record))
			{
				mAssetCache->ReturnAssetBuffer(record);
			}
		}

		r2::sarr::Clear(*mCachedRecords);

		mAssetCache->FlushAll();
	}

	r2::asset::AssetCacheRecord GameAssetManager::FindAssetCacheRecord(const r2::asset::AssetHandle& assetHandle)
	{
		const u32 numCacheRecords = r2::sarr::Size(*mCachedRecords);

		for (u32 i = 0; i < numCacheRecords; ++i)
		{
			const r2::asset::AssetCacheRecord& assetCacheRecord = r2::sarr::At(*mCachedRecords, i);
			if (assetCacheRecord.GetAsset().HashID() == assetHandle.handle)
			{
				return assetCacheRecord;
			}
		}

		return {};
	}

	void GameAssetManager::RemoveAssetCacheRecord(const r2::asset::AssetHandle& assetHandle)
	{
		const u32 numCacheRecords = r2::sarr::Size(*mCachedRecords);

		for (u32 i = 0; i < numCacheRecords; ++i)
		{
			const r2::asset::AssetCacheRecord& assetCacheRecord = r2::sarr::At(*mCachedRecords, i);
			if (assetCacheRecord.GetAsset().HashID() == assetHandle.handle)
			{
				r2::sarr::RemoveAndSwapWithLastElement(*mCachedRecords, i);
				break;
			}
		}
	}
}