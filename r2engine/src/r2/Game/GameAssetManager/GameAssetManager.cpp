#include "r2pch.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"
#include "r2/Utils/Hash.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace
{
	const u64 EMPTY_TEXTURE_PACK_NAME = STRING_ID("");
}

namespace r2
{
	
	GameAssetManager::GameAssetManager()
		:mAssetCache(nullptr)
		,mCachedRecords(nullptr)
		,mTexturePacksCache(nullptr)
	{
	}

	GameAssetManager::~GameAssetManager()
	{
		mAssetCache = nullptr;
		mCachedRecords = nullptr;
		mTexturePacksCache = nullptr;
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
		u64 memorySize = r2::asset::AssetCache::TotalMemoryNeeded(numAssets, assetCapacity, alignment, lruCapacity, mapCapacity);
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

	u64 GameAssetManager::GetAssetDataSize(r2::asset::AssetHandle assetHandle)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet!");
			return 0;
		}

		r2::asset::AssetCacheRecord defaultAssetCacheRecord;

		r2::asset::AssetCacheRecord result = r2::shashmap::Get(*mCachedRecords, assetHandle.handle, defaultAssetCacheRecord);

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(result))
		{
			return result.GetAssetBuffer()->Size();
		}

		result = mAssetCache->GetAssetBuffer(assetHandle);

		R2_CHECK(result.GetAssetBuffer()->IsLoaded(), "Not loaded?");

		//store the record
		r2::shashmap::Set(*mCachedRecords, assetHandle.handle, result);

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

	void AddTexturePacksToTexturePackSet(const flat::MaterialParams* materialParams, r2::SArray<u64>& texturePacks)
	{
		auto textureParams = materialParams->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			u64 texturePackName = textureParams->Get(i)->texturePackName();

			if (EMPTY_TEXTURE_PACK_NAME == texturePackName)
			{
				continue;
			}

			bool found = false;
			for (u32 j = 0; j < r2::sarr::Size(texturePacks); ++j)
			{
				if (r2::sarr::At(texturePacks, j) == texturePackName)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				r2::sarr::Push(texturePacks, texturePackName);
			}
		}
	}

	bool GameAssetManager::AddTexturePacksManifest(u64 texturePackManifestHandle, const flat::TexturePacksManifest* texturePacksManifest)
	{
		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		if (texturePacksManifest == nullptr || texturePackManifestHandle <= 0)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		return r2::draw::texche::AddTexturePacksManifestFile(*mTexturePacksCache, texturePackManifestHandle, texturePacksManifest).handle != draw::texche::TexturePacksManifestHandle::Invalid.handle;
	}

	bool GameAssetManager::LoadMaterialTextures(const flat::MaterialParams* materialParams)
	{
		if (materialParams == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}
		
		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		auto textureParams = materialParams->textureParams();
		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, textureParams->size());
		AddTexturePacksToTexturePackSet(materialParams, *texturePacks);

		/*

	
		r2::SArray<r2::draw::tex::Texture>* textures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, r2::draw::tex::Cubemap);
		r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, r2::draw::tex::Cubemap);

		if (texturePacks == nullptr)
		{
			R2_CHECK(false, "We couldn't create the texturePacks array");
			return false;
		}

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			r2::sarr::Push(*texturePacks, textureParams->Get(i)->texturePackName());
			if (!r2::draw::texche::IsTexturePackACubemap(*mTexturePacksCache, textureParams->Get(i)->texturePackName()))
			{
				const r2::draw::tex::Texture* texture = r2::draw::texche::GetTextureFromTexturePack(*mTexturePacksCache, textureParams->Get(i)->texturePackName(), textureParams->Get(i)->value());

				R2_CHECK(texture != nullptr, "Should never be nullptr");

				r2::sarr::Push(*textures, *texture);
			}
			else
			{
				const r2::draw::tex::CubemapTexture* cubemap = r2::draw::texche::GetCubemapTextureForTexturePack(*mTexturePacksCache, textureParams->Get(i)->texturePackName());

				R2_CHECK(cubemap != nullptr, "Should never be nullptr");

				r2::sarr::Push(*cubemaps, *cubemap);
			}
		}

		if (r2::sarr::Size(*textures) > 0)
		{
			draw::texche::LoadTextuesFromDisk(*mTexturePacksCache, *texturePacks, *textures);
		}
		else
		{
			draw::texche::LoadCubemapTexturesFromDisk(*mTexturePacksCache, *texturePacks, *cubemaps);
		}
		
		FREE(cubemaps, *MEM_ENG_SCRATCH_PTR);
		FREE(textures, *MEM_ENG_SCRATCH_PTR);
		*/
		draw::texche::LoadTexturePacksFromDisk(*mTexturePacksCache, *texturePacks);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	bool GameAssetManager::LoadMaterialTextures(const flat::MaterialParamsPack* materialParamsPack)
	{
		if (materialParamsPack == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		u32 numTexturePacks = materialParamsPack->pack()->size() * draw::tex::Cubemap;

		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, numTexturePacks);

		for (flatbuffers::uoffset_t i = 0; i < materialParamsPack->pack()->size(); ++i)
		{
			const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(i);
			AddTexturePacksToTexturePackSet(materialParams, *texturePacks);
		}

		draw::texche::LoadTexturePacksFromDisk(*mTexturePacksCache, *texturePacks);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);
		
		return true;
	}

	void GameAssetManager::GetTexturesForMaterialParamsInternal(r2::SArray<u64>* texturePacks, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{

		for (u32 i = 0; i < r2::sarr::Size(*texturePacks); ++i)
		{
			const u64 texturePackName = r2::sarr::At(*texturePacks, i);

			bool isCubemap = r2::draw::texche::IsTexturePackACubemap(*mTexturePacksCache, texturePackName);

			if (!isCubemap && textures != nullptr)
			{
				const r2::SArray<r2::draw::tex::Texture>* texturePackTextures = r2::draw::texche::GetTexturesForTexturePack(*mTexturePacksCache, texturePackName);
				R2_CHECK(texturePackTextures != nullptr, "");
				r2::sarr::Append(*textures, *texturePackTextures);
			}
			else if (isCubemap && cubemaps != nullptr)
			{
				const r2::draw::tex::CubemapTexture* cubemap = r2::draw::texche::GetCubemapTextureForTexturePack(*mTexturePacksCache, texturePackName);

				R2_CHECK(cubemap != nullptr, "");

				r2::sarr::Push(*cubemaps, *cubemap);
			}
			else
			{
				R2_CHECK(false, "hmm");
			}
		}
	}

	bool GameAssetManager::GetTexturesForMaterialParams(const flat::MaterialParams* materialParams, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{
		//for this method specifically, we only want to get the specific textures of this material, no others from the packs
		if (materialParams == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		auto textureParams = materialParams->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			if (textureParams->Get(i)->value() == EMPTY_TEXTURE_PACK_NAME)
			{
				continue;
			}

			if (!r2::draw::texche::IsTexturePackACubemap(*mTexturePacksCache, textureParams->Get(i)->texturePackName()))
			{
				const r2::draw::tex::Texture* texture = r2::draw::texche::GetTextureFromTexturePack(*mTexturePacksCache, textureParams->Get(i)->texturePackName(), textureParams->Get(i)->value());

				if (texture)
				{
					r2::sarr::Push(*textures, *texture);
				}
			}
			else
			{
				const r2::draw::tex::CubemapTexture* cubemap = r2::draw::texche::GetCubemapTextureForTexturePack(*mTexturePacksCache, textureParams->Get(i)->texturePackName());

				R2_CHECK(cubemap != nullptr, "Should never be nullptr");

				r2::sarr::Push(*cubemaps, *cubemap);
			}
		}

		return true;
	}

	bool GameAssetManager::GetTexturesForMaterialParamsPack(const flat::MaterialParamsPack* materialParamsPack, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{
		if (materialParamsPack == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, materialParamsPack->pack()->size() * draw::tex::Cubemap);

		for (flatbuffers::uoffset_t i = 0; i < materialParamsPack->pack()->size(); ++i)
		{
			const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(i);
			AddTexturePacksToTexturePackSet(materialParams, *texturePacks);
		}

		GetTexturesForMaterialParamsInternal(texturePacks, textures, cubemaps);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	const r2::draw::tex::Texture* GameAssetManager::GetAlbedoTextureForMaterialName(const flat::MaterialParamsPack* materialParamsPack, u64 materialName)
	{
		s32 materialParamsPackIndex = -1;
		s32 materialTexParamsIndex = -1;

		for (flatbuffers::uoffset_t i = 0; i < materialParamsPack->pack()->size(); ++i)
		{
			const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(i);
			if (materialParams->name() == materialName)
			{
				const auto numTextureParams = materialParams->textureParams()->size();
				
				for (u32 j = 0; j < numTextureParams; ++j)
				{
					if (materialParams->textureParams()->Get(j)->propertyType() == flat::MaterialPropertyType_ALBEDO)
					{
						materialParamsPackIndex = i;
						materialTexParamsIndex = j;
						break;
					}
				}

				if (materialParamsPackIndex != -1)
				{
					break;
				}
			}
		}

		if (materialParamsPackIndex == -1 || materialTexParamsIndex == -1)
		{
			return nullptr;
		}

		const r2::SArray<r2::draw::tex::Texture>* textures =draw::texche::GetTexturesForTexturePack(*mTexturePacksCache, materialParamsPack->pack()->Get(materialParamsPackIndex)->textureParams()->Get(materialTexParamsIndex)->texturePackName());

		const auto numTextures = r2::sarr::Size(*textures);

		for (u32 i = 0; i < numTextures; ++i)
		{
			const draw::tex::Texture& texture = r2::sarr::At(*textures, i);
			if (texture.type == draw::tex::Diffuse)
			{
				return &texture;
			}
		}

		return nullptr;
	}

	const r2::draw::tex::CubemapTexture* GameAssetManager::GetCubemapTextureForMaterialName(const flat::MaterialParamsPack* materialParamsPack, u64 materialName)
	{
		s32 materialParamsPackIndex = -1;
		s32 materialTexParamsIndex = -1;

		for (flatbuffers::uoffset_t i = 0; i < materialParamsPack->pack()->size(); ++i)
		{
			const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(i);
			if (materialParams->name() == materialName)
			{
				const auto numTextureParams = materialParams->textureParams()->size();

				for (u32 j = 0; j < numTextureParams; ++j)
				{
					if (materialParams->textureParams()->Get(j)->propertyType() == flat::MaterialPropertyType_ALBEDO)
					{
						materialParamsPackIndex = i;
						materialTexParamsIndex = j;
						break;
					}
				}

				if (materialParamsPackIndex != -1)
				{
					break;
				}
			}
		}

		if (materialParamsPackIndex == -1 || materialTexParamsIndex == -1)
		{
			return nullptr;
		}

		return r2::draw::texche::GetCubemapTextureForTexturePack(*mTexturePacksCache, materialParamsPack->pack()->Get(materialParamsPackIndex)->textureParams()->Get(materialTexParamsIndex)->texturePackName());
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
		newEntry.reloadType = r2::asset::CHANGED;
		newEntry.filePath = path;

		mHotReloadQueue.Push(newEntry);
	}

	void GameAssetManager::AssetAdded(const std::string& path, r2::asset::AssetType type)
	{
		HotReloadEntry newEntry;
		newEntry.assetType = type;
		newEntry.reloadType = r2::asset::ADDED;
		newEntry.filePath = path;

		mHotReloadQueue.Push(newEntry);
	}

	void GameAssetManager::AssetRemoved(const std::string& path, r2::asset::AssetType type)
	{
		HotReloadEntry newEntry;
		newEntry.assetType = type;
		newEntry.reloadType = r2::asset::DELETED;
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

	std::vector<r2::asset::AssetFile*> GameAssetManager::GetAllAssetFilesForAssetType(r2::asset::AssetType type)
	{
		if (!mAssetCache)
		{
			R2_CHECK(false, "Asset Cache is nullptr");
			return {};
		}

		return mAssetCache->GetAllAssetFilesForAssetType(type);
	}

#endif


}