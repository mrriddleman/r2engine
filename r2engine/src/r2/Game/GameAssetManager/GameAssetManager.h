#ifndef __GAME_ASSET_MANAGER_H__
#define __GAME_ASSET_MANAGER_H__


#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetCacheRecord.h"

#include "r2/Core/Memory/Memory.h"

#ifdef R2_ASSET_PIPELINE
#include <string>
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#endif

namespace r2::asset
{
	class AssetLoader;
	class AssetWriter;
	class AssetCache;
}

namespace r2
{
	class GameAssetManager
	{
	public:
		GameAssetManager();
		~GameAssetManager();

		//No copy or move semantics


		bool Init(r2::mem::MemoryArea::Handle assetMemoryHandle, r2::asset::FileList fileList);
		void Shutdown();

		void Update();

		static u64 MemorySizeForGameAssetManager(u32 alignment, u32 headerSize);
		static u64 CacheMemorySize(u32 numAssets, u32 assetCapacity, u32 alignment, u32 headerSize, u32 boundsChecking, u32 lruCapacity, u32 mapCapacity);

		//@TODO(Serge): probably want more types of loads for threading and stuff, for now keep it simple
		r2::asset::AssetHandle LoadAsset(const r2::asset::Asset& asset);
		r2::asset::AssetCacheRecord GetAssetData(r2::asset::AssetHandle assetHandle);
		void ReturnAssetData(const r2::asset::AssetCacheRecord& assetCacheRecord);

		r2::asset::AssetHandle ReloadAsset(const r2::asset::Asset& asset);
		void FreeAsset(const r2::asset::AssetHandle& handle);

		void RegisterAssetLoader(r2::asset::AssetLoader* assetLoader);
		void RegisterAssetWriter(r2::asset::AssetWriter* assetWriter);
		void RegisterAssetFreedCallback(r2::asset::AssetFreedCallback func);

#ifdef R2_ASSET_PIPELINE
		void AssetChanged(const std::string& path, r2::asset::AssetType type);
		void AssetAdded(const std::string& path, r2::asset::AssetType type);
		void AssetRemoved(const std::string& path, r2::asset::AssetType type);

		void RegisterReloadFunction(r2::asset::AssetReloadedFunc func);

		void AddAssetFile(r2::asset::AssetFile* assetFile);
		void RemoveAssetFile(const std::string& filePath);
#endif

	private:

		r2::asset::AssetCache* mAssetCache;
		r2::mem::MemoryArea::Handle mAssetMemoryHandle;

#ifdef R2_ASSET_PIPELINE

		enum HotReloadType :u32
		{
			CHANGED = 0,
			ADDED,
			DELETED
		};

		struct HotReloadEntry
		{
			std::string filePath;
			r2::asset::AssetType assetType;
			HotReloadType reloadType;
		};

		r2::asset::pln::AssetThreadSafeQueue<HotReloadEntry> mHotReloadQueue;
#endif
	};


	
}


#endif
