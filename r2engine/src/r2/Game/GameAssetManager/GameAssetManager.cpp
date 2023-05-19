#include "r2pch.h"

#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetLoaders/MeshAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/ModelAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RAnimationAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"

namespace r2
{
	
	GameAssetManager::GameAssetManager()
		:mAssetCache(nullptr)
	{
	}

	GameAssetManager::~GameAssetManager()
	{
		mAssetCache = nullptr;
	}

	bool GameAssetManager::Init(r2::mem::utils::MemBoundary assetBoundary, r2::asset::FileList fileList)
	{
		mAssetCache = r2::asset::lib::CreateAssetCache(assetBoundary, fileList);

		//@TODO(Serge): maybe add the loaders here for the engine?
		r2::asset::MeshAssetLoader* meshLoader = (r2::asset::MeshAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::MeshAssetLoader>();
		mAssetCache->RegisterAssetLoader(meshLoader);

		r2::asset::ModelAssetLoader* modelLoader = (r2::asset::ModelAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::ModelAssetLoader>();
		modelLoader->SetAssetCache(mAssetCache); //@TODO(Serge): make some callbacks or something here instead

		mAssetCache->RegisterAssetLoader(modelLoader);

		r2::asset::RModelAssetLoader* rmodelLoader = (r2::asset::RModelAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::RModelAssetLoader>();
		mAssetCache->RegisterAssetLoader(rmodelLoader);

		r2::asset::RAnimationAssetLoader* ranimationLoader = (r2::asset::RAnimationAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::RAnimationAssetLoader>();
		mAssetCache->RegisterAssetLoader(ranimationLoader);

		return mAssetCache != nullptr;
	}

	void GameAssetManager::Shutdown()
	{
		if (mAssetCache)
		{
			r2::asset::lib::DestroyCache(mAssetCache);
		}
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

	u64 GameAssetManager::MemorySizeForGameAssetManager(u32 alignment, u32 headerSize)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		u64 memorySize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(GameAssetManager), alignment, headerSize, boundsChecking);

		return memorySize;
	}

	u64 GameAssetManager::CacheMemorySize(u32 numAssets, u32 assetCapacity, u32 alignment, u32 headerSize, u32 boundsChecking, u32 lruCapacity, u32 mapCapacity)
	{
		u64 memorySize = r2::asset::AssetCache::TotalMemoryNeeded(headerSize, boundsChecking, numAssets, assetCapacity, alignment, lruCapacity, mapCapacity);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::asset::AssetCache), alignment, headerSize, boundsChecking);
		return memorySize;
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

	r2::asset::AssetCacheRecord GameAssetManager::GetAssetData(r2::asset::AssetHandle assetHandle)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return {};
		}

		return mAssetCache->GetAssetBuffer(assetHandle);
	}

	void GameAssetManager::ReturnAssetData(const r2::asset::AssetCacheRecord& assetCacheRecord)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		mAssetCache->ReturnAssetBuffer(assetCacheRecord);
	}

	r2::asset::AssetHandle GameAssetManager::ReloadAsset(const r2::asset::Asset& asset)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return {};
		}

		return mAssetCache->ReloadAsset(asset);
	}

	void GameAssetManager::FreeAsset(const r2::asset::AssetHandle& handle)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return;
		}

		mAssetCache->FreeAsset(handle);
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