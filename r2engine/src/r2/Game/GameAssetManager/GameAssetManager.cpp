#include "r2pch.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"


namespace r2
{
	
	GameAssetManager::GameAssetManager()
		:mAssetCache(nullptr)
		,mCachedRecords(nullptr)
		,mAssetMemoryHandle()
	{
	}

	GameAssetManager::~GameAssetManager()
	{
		mAssetCache = nullptr;
	}

	void GameAssetManager::Update()
	{
#ifdef R2_ASSET_PIPELINE

		HotReloadEntry nextEntry;
		while (mAssetCache && mHotReloadQueue.TryPop(nextEntry))
		{
			//@TODO(Serge): process the entry
		}
#endif
	}

	u64 GameAssetManager::MemorySizeForGameAssetManager(u32 numFiles, u32 alignment, u32 headerSize)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		u64 memorySize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(GameAssetManager), alignment, headerSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::asset::AssetCacheRecord>::MemorySize(numFiles * r2::SHashMap<u32>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking);

		return memorySize;
	}

	u64 GameAssetManager::CacheMemorySize(u32 numAssets, u32 assetCapacity, u32 alignment, u32 headerSize, u32 boundsChecking, u32 lruCapacity, u32 mapCapacity)
	{
		u64 memorySize = r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking, numAssets, assetCapacity, alignment, lruCapacity, mapCapacity);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::asset::AssetCache), alignment, headerSize, boundsChecking);
		return memorySize;
	}

	bool GameAssetManager::HasAsset(const r2::asset::Asset& asset)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return false;
		}

		return mAssetCache->HasAsset(asset);
	}

	const r2::asset::FileList GameAssetManager::GetFileList() const
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return false;
		}

		return mAssetCache->GetFileList();
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

	void GameAssetManager::UnloadAsset(const r2::asset::AssetHandle& assetHandle)
	{
		if (!mAssetCache || !mCachedRecords)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		r2::asset::AssetCacheRecord defaultAssetCacheRecord;
		r2::asset::AssetCacheRecord result = r2::shashmap::Get(*mCachedRecords, assetHandle.handle, defaultAssetCacheRecord);

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
		{
			r2::shashmap::Remove(*mCachedRecords, assetHandle.handle);

			R2_CHECK(!r2::shashmap::Has(*mCachedRecords, assetHandle.handle), "We still have the asset record?");

			bool wasReturned = mAssetCache->ReturnAssetBuffer(result);

			R2_CHECK(wasReturned, "Somehow we couldn't return the asset cache record");
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
		r2::asset::AssetCacheRecord result = r2::shashmap::Get(*mCachedRecords, asset.HashID(), defaultAssetCacheRecord);

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
		{
			r2::shashmap::Remove(*mCachedRecords, asset.HashID());

			R2_CHECK(!r2::shashmap::Has(*mCachedRecords, asset.HashID()), "We still have the asset record?");

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

	void GameAssetManager::RegisterAssetWriter(r2::asset::AssetWriter* assetWriter)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		mAssetCache->RegisterAssetWriter(assetWriter);
	}

	void GameAssetManager::RegisterAssetFreedCallback(r2::asset::AssetFreedCallback func)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		mAssetCache->RegisterAssetFreedCallback(func);
	}

	void GameAssetManager::FreeAllAssets()
	{
		if (!mAssetCache || !mCachedRecords)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		auto iter = r2::shashmap::Begin(*mCachedRecords);
		for (; iter != r2::shashmap::End(*mCachedRecords); ++iter)
		{
			mAssetCache->ReturnAssetBuffer(iter->value);
		}

		r2::shashmap::Clear(*mCachedRecords);

		mAssetCache->FlushAll();
	}

#ifdef R2_ASSET_PIPELINE

	void GameAssetManager::AssetChanged(const std::string& path, r2::asset::AssetType type)
	{
		HotReloadEntry newEntry;
		newEntry.assetType = type;
		newEntry.reloadType = CHANGED;
		newEntry.filePath = path;

		mHotReloadQueue.Push(newEntry);
	}

	void GameAssetManager::AssetAdded(const std::string& path, r2::asset::AssetType type)
	{
		HotReloadEntry newEntry;
		newEntry.assetType = type;
		newEntry.reloadType = ADDED;
		newEntry.filePath = path;

		mHotReloadQueue.Push(newEntry);
	}

	void GameAssetManager::AssetRemoved(const std::string& path, r2::asset::AssetType type)
	{
		HotReloadEntry newEntry;
		newEntry.assetType = type;
		newEntry.reloadType = DELETED;
		newEntry.filePath = path;

		mHotReloadQueue.Push(newEntry);
	}

	void GameAssetManager::RegisterReloadFunction(r2::asset::AssetReloadedFunc func)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		mAssetCache->AddReloadFunction(func);
	}

	void GameAssetManager::AddAssetFile(r2::asset::AssetFile* assetFile)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		r2::asset::FileList fileList = mAssetCache->GetFileList();
		r2::sarr::Push(*fileList, assetFile);
	}

	void GameAssetManager::RemoveAssetFile(const std::string& filePath)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		//find it first
		r2::asset::FileList fileList = mAssetCache->GetFileList();

		const u32 numFiles = r2::sarr::Size(*fileList);

		for (u32 i = 0; i < numFiles; ++i)
		{
			const r2::asset::AssetFile* nextAssetFile = r2::sarr::At(*fileList, i);

			if (std::string(nextAssetFile->FilePath()) == filePath)
			{
				r2::sarr::RemoveAndSwapWithLastElement(*fileList, i);
				break;
			}
		}
	}

#endif


}