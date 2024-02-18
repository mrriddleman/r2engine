#ifndef __GAME_ASSET_MANAGER_H__
#define __GAME_ASSET_MANAGER_H__


#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetCacheRecord.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetLoaders/MeshAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/ModelAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/SoundAssetLoader.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Model/Textures/TexturePacksCache.h"

#include <cstdio>

namespace r2::asset
{
	class AssetLoader;
	class AssetWriter;
	class AssetCache;
}

namespace flat
{
	struct MaterialPack;
	struct Material;
}

namespace r2
{
	class GameAssetManager
	{
	public:
		GameAssetManager();
		~GameAssetManager();

		template<class ARENA>
		bool Init(
			ARENA& arena,
			r2::mem::MemoryArea::Handle assetMemoryHandle,
			r2::mem::MemoryArea::SubArea::Handle assetSubMemoryHandle,
			u32 assetCacheSize)
		{
			mMemoryAreaHandle = assetMemoryHandle;
			mMemorySubeAreaHandle = assetSubMemoryHandle;

			r2::mem::MemoryArea* gameAssetManagerMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(assetMemoryHandle);
			R2_CHECK(gameAssetManagerMemoryArea != nullptr, "Failed to get the memory area!");

			const u64 fileListCapacity = r2::asset::lib::MAX_NUM_GAME_ASSET_FILES;

			auto areaBoundary = gameAssetManagerMemoryArea->SubAreaBoundary(mMemorySubeAreaHandle);
			areaBoundary.alignment = 16;

			mAssetCache = r2::asset::lib::CreateAssetCache(areaBoundary, assetCacheSize, fileListCapacity);

			mCachedRecords = MAKE_SARRAY(arena, r2::asset::AssetCacheRecord, fileListCapacity);

			R2_CHECK(mCachedRecords != nullptr, "couldn't create the cached records");

			r2::asset::MeshAssetLoader* meshLoader = (r2::asset::MeshAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::MeshAssetLoader>();
			mAssetCache->RegisterAssetLoader(meshLoader);

			r2::asset::ModelAssetLoader* modelLoader = (r2::asset::ModelAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::ModelAssetLoader>();
			modelLoader->SetAssetCache(mAssetCache); //@TODO(Serge): make some callbacks or something here instead

			mAssetCache->RegisterAssetLoader(modelLoader);

			r2::asset::RModelAssetLoader* rmodelLoader = (r2::asset::RModelAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::RModelAssetLoader>();
			mAssetCache->RegisterAssetLoader(rmodelLoader);

			r2::asset::SoundAssetLoader* soundLoader = (r2::asset::SoundAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::SoundAssetLoader>();
			mAssetCache->RegisterAssetLoader(soundLoader);

			return mAssetCache != nullptr;
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{

			FreeAllAssets();

			if (mCachedRecords)
			{
				FREE(mCachedRecords, arena);
			}

			if (mAssetCache)
			{
				r2::asset::lib::DestroyCache(mAssetCache);
			}
		}

		static u64 MemorySizeForGameAssetManager(u32 numFiles, u32 alignment, u32 headerSize);
		static u64 CacheMemorySize(u32 numAssets, u32 assetCapacity, u32 alignment, u32 headerSize, u32 boundsChecking, u32 lruCapacity, u32 mapCapacity);

		//@TODO(Serge): probably want more types of loads for threading and stuff, for now keep it simple
		r2::asset::AssetHandle LoadAsset(const r2::asset::Asset& asset);

		template<typename T>
		T* LoadAndGetAsset(const r2::asset::Asset& asset)
		{
			r2::asset::AssetHandle handle = LoadAsset(asset);
			return GetAssetData<T>(handle);
		}

		template<typename T>
		const T* LoadAndGetAssetConst(const r2::asset::Asset& asset)
		{
			return LoadAndGetAsset<T>(asset);
		}

		//@NOTE(Serge): POTENTIAL BIG ISSUE - if we have handles that are equal even though their asset types are different, this
		//				will cause a hash conflict. Must be mindful of this. The error should be fairly obvious since it will probably
		//				be a corruption when you try to get the asset. We could make our own (for this class) kind of asset handle
		//				that would just index into an array or something but keep it simple for now.
		template<typename T>
		T* GetAssetData(r2::asset::AssetHandle assetHandle)
		{
			if (!mAssetCache)
			{
				R2_CHECK(false, "We haven't initialized the GameAssetManager yet!");
				return nullptr;
			}

			r2::asset::AssetCacheRecord result = FindAssetCacheRecord(assetHandle);

			if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
			{
				return reinterpret_cast<T*>(result.GetAssetBuffer()->MutableData());
			}

			result = mAssetCache->GetAssetBuffer(assetHandle);

			if (result.GetAssetBuffer() == nullptr || !result.GetAssetBuffer()->IsLoaded())
			{
				return nullptr;
			}
		//	R2_CHECK(result.GetAssetBuffer()->IsLoaded(), "Not loaded?");

			//store the record
			r2::sarr::Push(*mCachedRecords, result);


			return reinterpret_cast<T*>(result.GetAssetBuffer()->MutableData());
		}

		template<typename T>
		T* GetAssetData(u64 assetName)
		{
			return GetAssetData<T>({ assetName, GetAssetCacheSlot() });
		}

		template<typename T>
		const T* GetAssetDataConst(r2::asset::AssetHandle assetHandle)
		{
			return GetAssetData<T>(assetHandle);
		}

		template<typename T>
		const T* GetAssetDataConst(u64 assetName)
		{
			return GetAssetDataConst<T>({ assetName, GetAssetCacheSlot() });
		}

		template<typename T>
		const T* GetAssetDataConst(r2::asset::AssetName assetName)
		{
			return GetAssetDataConst<T>({ assetName.hashID, GetAssetCacheSlot() });
		}

		u64 GetAssetDataSize(r2::asset::AssetHandle assetHandle);

		void UnloadAsset(const r2::asset::AssetHandle& assetHandle);
		void UnloadAsset(const u64 assethandle);

		void UnloadAsset(const r2::asset::AssetName& assetName);

		r2::asset::AssetHandle ReloadAsset(const r2::asset::Asset& asset);

		void RegisterAssetLoader(r2::asset::AssetLoader* assetLoader);

		s64 GetAssetCacheSlot() const;

	private:

		void FreeAllAssets();
		
		r2::asset::AssetCacheRecord FindAssetCacheRecord(const r2::asset::AssetHandle& assetHandle);
		void RemoveAssetCacheRecord(const r2::asset::AssetHandle& assetHandle);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mMemorySubeAreaHandle;

		r2::asset::AssetCache* mAssetCache;

		//@NOTE(Serge): POTENTIAL BIG ISSUE - if we have handles that are equal even though their asset types are different, this
		//				will cause a hash conflict. Must be mindful of this. The error should be fairly obvious since it will probably
		//				be a corruption when you try to get the asset. We could make our own (for this class) kind of asset handle
		//				that would just index into an array or something but keep it simple for now.
		r2::SArray<r2::asset::AssetCacheRecord>* mCachedRecords;
	};
}

#endif
