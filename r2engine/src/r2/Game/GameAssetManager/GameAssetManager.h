#ifndef __GAME_ASSET_MANAGER_H__
#define __GAME_ASSET_MANAGER_H__


#include "r2/Core/Assets/Asset.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetCacheRecord.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetLoaders/MeshAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/ModelAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RAnimationAssetLoader.h"
#include "r2/Core/Assets/AssetLoaders/RModelAssetLoader.h"

#include "r2/Core/Memory/Memory.h"

#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/Textures/TexturePacksCache.h"

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

namespace flat
{
	struct MaterialParamsPack;
	struct MaterialParams;
}

namespace r2
{
	class GameAssetManager
	{
	public:
		GameAssetManager();
		~GameAssetManager();

		template<class ARENA>
		bool Init(ARENA& arena, r2::mem::MemoryArea::Handle assetMemoryHandle, r2::asset::FileList fileList, u32 numTextures, u32 numTextureManifests, u32 numTexturePacks)
		{
			r2::mem::MemoryArea* gameAssetManagerMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(assetMemoryHandle);
			R2_CHECK(gameAssetManagerMemoryArea != nullptr, "Failed to get the memory area!");

			mAssetCache = r2::asset::lib::CreateAssetCache(gameAssetManagerMemoryArea->AreaBoundary(), fileList);

			const u64 fileListCapacity = r2::sarr::Capacity(*fileList);

			mCachedRecords = MAKE_SARRAY(arena, r2::asset::AssetCacheRecord, fileListCapacity);

			R2_CHECK(mCachedRecords != nullptr, "couldn't create the cached records");

			r2::asset::MeshAssetLoader* meshLoader = (r2::asset::MeshAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::MeshAssetLoader>();
			mAssetCache->RegisterAssetLoader(meshLoader);

			r2::asset::ModelAssetLoader* modelLoader = (r2::asset::ModelAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::ModelAssetLoader>();
			modelLoader->SetAssetCache(mAssetCache); //@TODO(Serge): make some callbacks or something here instead

			mAssetCache->RegisterAssetLoader(modelLoader);

			r2::asset::RModelAssetLoader* rmodelLoader = (r2::asset::RModelAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::RModelAssetLoader>();
			mAssetCache->RegisterAssetLoader(rmodelLoader);

			r2::asset::RAnimationAssetLoader* ranimationLoader = (r2::asset::RAnimationAssetLoader*)mAssetCache->MakeAssetLoader<r2::asset::RAnimationAssetLoader>();
			mAssetCache->RegisterAssetLoader(ranimationLoader);

			mTexturePacksCache = draw::texche::Create<ARENA>(arena, numTextures, numTextureManifests, numTexturePacks, this);

			R2_CHECK(mTexturePacksCache != nullptr, "Failed to make the texture packs cache");

			return mAssetCache != nullptr;
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			r2::draw::texche::Shutdown<ARENA>(arena, mTexturePacksCache);

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

		void Update();

		static u64 MemorySizeForGameAssetManager(u32 numFiles, u32 alignment, u32 headerSize);
		static u64 CacheMemorySize(u32 numAssets, u32 assetCapacity, u32 alignment, u32 headerSize, u32 boundsChecking, u32 lruCapacity, u32 mapCapacity);

		bool HasAsset(const r2::asset::Asset& asset);
		const r2::asset::FileList GetFileList() const;

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

			r2::asset::AssetCacheRecord result = FindAssetCacheRecord(assetHandle);//r2::shashmap::Get(*mCachedRecords, assetHandle.handle, defaultAssetCacheRecord);

			if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
			{
				return reinterpret_cast<T*>(result.GetAssetBuffer()->MutableData());
			}

			result = mAssetCache->GetAssetBuffer(assetHandle);

			R2_CHECK(result.GetAssetBuffer()->IsLoaded(), "Not loaded?");

			//store the record
			//r2::shashmap::Set(*mCachedRecords, assetHandle.handle, result);
			r2::sarr::Push(*mCachedRecords, result);


			return reinterpret_cast<T*>(result.GetAssetBuffer()->MutableData());
		}

		template<typename T>
		const T* GetAssetDataConst(r2::asset::AssetHandle assetHandle)
		{
			return GetAssetData<T>(assetHandle);
		}

		u64 GetAssetDataSize(r2::asset::AssetHandle assetHandle);

		void UnloadAsset(const r2::asset::AssetHandle& assetHandle);
		void UnloadAsset(const u64 assethandle);

		r2::asset::AssetHandle ReloadAsset(const r2::asset::Asset& asset);

		void RegisterAssetLoader(r2::asset::AssetLoader* assetLoader);
		void RegisterAssetWriter(r2::asset::AssetWriter* assetWriter);
		void RegisterAssetFreedCallback(r2::asset::AssetFreedCallback func);

		//@TODO(Serge): really annoying we have two different interfaces - 1 for textures, 1 for everything else
		//				we need a to figure out a way to consolidate these. I think we'd have to pack all the textures together into 1 
		//				file for this to work
		bool AddTexturePacksManifest(u64 texturePackManifestHandle, const flat::TexturePacksManifest* texturePacksManifest);
		bool UpdateTexturePacksManifest(u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest);
		
		bool LoadMaterialTextures(const flat::MaterialParams* materialParams);
		bool LoadMaterialTextures(const flat::MaterialParamsPack* materialParamsPack);
		
		bool UnloadTexturePack(u64 texturePackName);

		bool GetTexturesForMaterialParams(const flat::MaterialParams* materialParams, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);
		bool GetTexturesForMaterialParamsPack(const flat::MaterialParamsPack* materialParamsPack, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);

		const r2::draw::tex::Texture* GetAlbedoTextureForMaterialName(const flat::MaterialParamsPack* materialParamsPack, u64 materialName);
		const r2::draw::tex::CubemapTexture* GetCubemapTextureForMaterialName(const flat::MaterialParamsPack* materialParamsPack, u64 materialName);

#ifdef R2_ASSET_PIPELINE

		bool ReloadTextureInTexturePack(u64 texturePackName, u64 textureName);
		bool ReloadTexturePack(u64 texturePackName);

		void AddAssetFile(r2::asset::AssetFile* assetFile);
		void RemoveAssetFile(const std::string& filePath);

		std::vector<r2::asset::AssetFile*> GetAllAssetFilesForAssetType(r2::asset::AssetType type);
		const r2::asset::AssetFile* GetAssetFile(const r2::asset::Asset& asset);
#endif

	private:

		void FreeAllAssets();
		void GetTexturesForMaterialParamsInternal(r2::SArray<u64>* texturePacks, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);

		r2::asset::AssetCacheRecord FindAssetCacheRecord(const r2::asset::AssetHandle& assetHandle);
		void RemoveAssetCacheRecord(const r2::asset::AssetHandle& assetHandle);

		r2::asset::AssetCache* mAssetCache;

		//@NOTE(Serge): POTENTIAL BIG ISSUE - if we have handles that are equal even though their asset types are different, this
		//				will cause a hash conflict. Must be mindful of this. The error should be fairly obvious since it will probably
		//				be a corruption when you try to get the asset. We could make our own (for this class) kind of asset handle
		//				that would just index into an array or something but keep it simple for now.
		r2::SArray<r2::asset::AssetCacheRecord>* mCachedRecords;

		r2::draw::TexturePacksCache* mTexturePacksCache;

	};
}

#endif
