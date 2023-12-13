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

#include "r2/Core/Memory/Memory.h"

#include "r2/Render/Model/Textures/TexturePacksCache.h"

#ifdef R2_ASSET_PIPELINE
#include <string>
#include "r2/Core/Assets/Pipeline/AssetThreadSafeQueue.h"
#endif

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
			u32 numTextures,
			u32 numTextureManifests,
			u32 numTexturePacks,
			u32 assetCacheSize)
		{
			r2::mem::MemoryArea* gameAssetManagerMemoryArea = r2::mem::GlobalMemory::GetMemoryArea(assetMemoryHandle);
			R2_CHECK(gameAssetManagerMemoryArea != nullptr, "Failed to get the memory area!");

			const u64 fileListCapacity = r2::asset::lib::MAX_NUM_GAME_ASSET_FILES;

			auto areaBoundary = gameAssetManagerMemoryArea->AreaBoundary();
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

			R2_CHECK(result.GetAssetBuffer()->IsLoaded(), "Not loaded?");

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

		u64 GetAssetDataSize(r2::asset::AssetHandle assetHandle);

		void UnloadAsset(const r2::asset::AssetHandle& assetHandle);
		void UnloadAsset(const u64 assethandle);

		r2::asset::AssetHandle ReloadAsset(const r2::asset::Asset& asset);

		void RegisterAssetLoader(r2::asset::AssetLoader* assetLoader);


		//@TODO(Serge): really annoying we have two different interfaces - 1 for textures, 1 for everything else
		//				we need a to figure out a way to consolidate these. I think we'd have to pack all the textures together into 1 
		//				file for this to work
		bool AddTexturePacksManifest(u64 texturePackManifestHandle, const flat::TexturePacksManifest* texturePacksManifest);
		bool UpdateTexturePacksManifest(u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest);
		
		bool LoadMaterialTextures(const flat::Material* material);
		bool LoadMaterialTextures(const flat::MaterialPack* materialPack);
		
		bool UnloadTexturePack(u64 texturePackName);
		
		bool GetTexturesForFlatMaterial(const flat::Material* material, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);
		bool GetTexturesForMaterialPack(const flat::MaterialPack* materialParamsPack, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);

		const r2::draw::tex::Texture* GetAlbedoTextureForMaterialName(const flat::MaterialPack* materialParamsPack, u64 materialName);
		const r2::draw::tex::CubemapTexture* GetCubemapTextureForMaterialName(const flat::MaterialPack* materialParamsPack, u64 materialName);

#if defined(R2_ASSET_PIPELINE) || defined(R2_EDITOR)

		bool ReloadTextureInTexturePack(u64 texturePackName, u64 textureName);
		bool ReloadTexturePack(u64 texturePackName);
#endif
		s64 GetAssetCacheSlot() const;

	private:

		void FreeAllAssets();
		void GetTexturesForMaterialnternal(r2::SArray<u64>* texturePacks, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);

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
