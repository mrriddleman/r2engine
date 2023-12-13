#include "r2pch.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"
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

	void AddTexturePacksToTexturePackSet(const flat::Material* material, r2::SArray<u64>& texturePacks)
	{
		auto textureParams = material->shaderParams()->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			u64 texturePackName = textureParams->Get(i)->texturePack()->assetName();

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

	bool GameAssetManager::UpdateTexturePacksManifest(u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest)
	{
		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		if (texturePacksManifest == nullptr || texturePackHandle <= 0)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		return r2::draw::texche::UpdateTexturePacksManifest(*mTexturePacksCache, texturePackHandle, texturePacksManifest);
	}

	bool GameAssetManager::LoadMaterialTextures(const flat::Material* material)
	{
		if (material == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}
		
		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		auto textureParams = material->shaderParams()->textureParams();
		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, textureParams->size());
		AddTexturePacksToTexturePackSet(material, *texturePacks);
		draw::texche::LoadTexturePacks(*mTexturePacksCache, *texturePacks);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	bool GameAssetManager::LoadMaterialTextures(const flat::MaterialPack* materialPack)
	{
		if (materialPack == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		u32 numTexturePacks = materialPack->pack()->size() * draw::tex::NUM_TEXTURE_TYPES;

		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, numTexturePacks);

		for (flatbuffers::uoffset_t i = 0; i < materialPack->pack()->size(); ++i)
		{
			const flat::Material* materialParams = materialPack->pack()->Get(i);
			AddTexturePacksToTexturePackSet(materialParams, *texturePacks);
		}

		draw::texche::LoadTexturePacks(*mTexturePacksCache, *texturePacks);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);
		
		return true;
	}

	bool GameAssetManager::UnloadTexturePack(u64 texturePackName)
	{
		return draw::texche::UnloadTexturePack(*mTexturePacksCache, texturePackName);
	}

	void GameAssetManager::GetTexturesForMaterialnternal(r2::SArray<u64>* texturePacks, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
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

	bool GameAssetManager::GetTexturesForFlatMaterial(const flat::Material* material, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{
		//for this method specifically, we only want to get the specific textures of this material, no others from the packs
		if (material == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		auto textureParams = material->shaderParams()->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			if (textureParams->Get(i)->value()->assetName() == EMPTY_TEXTURE_PACK_NAME)
			{
				continue;
			}

			if (!r2::draw::texche::IsTexturePackACubemap(*mTexturePacksCache, textureParams->Get(i)->texturePack()->assetName()))
			{
				const r2::draw::tex::Texture* texture = r2::draw::texche::GetTextureFromTexturePack(*mTexturePacksCache, textureParams->Get(i)->texturePack()->assetName(), textureParams->Get(i)->value()->assetName());

				if (texture)
				{
					r2::sarr::Push(*textures, *texture);
				}
			}
			else
			{
				const r2::draw::tex::CubemapTexture* cubemap = r2::draw::texche::GetCubemapTextureForTexturePack(*mTexturePacksCache, textureParams->Get(i)->texturePack()->assetName());

				R2_CHECK(cubemap != nullptr, "Should never be nullptr");

				r2::sarr::Push(*cubemaps, *cubemap);
			}
		}

		return true;
	}

	bool GameAssetManager::GetTexturesForMaterialPack(const flat::MaterialPack* materialPack, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{
		if (materialPack == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		if (mAssetCache == nullptr || mTexturePacksCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the GameAssetManager yet");
			return false;
		}

		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, materialPack->pack()->size() * draw::tex::NUM_TEXTURE_TYPES);

		for (flatbuffers::uoffset_t i = 0; i < materialPack->pack()->size(); ++i)
		{
			const flat::Material* material = materialPack->pack()->Get(i);
			AddTexturePacksToTexturePackSet(material, *texturePacks);
		}

		GetTexturesForMaterialnternal(texturePacks, textures, cubemaps);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	const r2::draw::tex::Texture* GameAssetManager::GetAlbedoTextureForMaterialName(const flat::MaterialPack* materialPack, u64 materialName)
	{
		s32 materialParamsPackIndex = -1;
		s32 materialTexParamsIndex = -1;
		u64 textureName = 0;

		for (flatbuffers::uoffset_t i = 0; i < materialPack->pack()->size(); ++i)
		{
			const flat::Material* material = materialPack->pack()->Get(i);
			if (material->assetName()->assetName() == materialName)
			{
				const auto* textureParams = material->shaderParams()->textureParams();
				const auto numTextureParams = textureParams->size();
				
				for (u32 j = 0; j < numTextureParams; ++j)
				{
					if (textureParams->Get(j)->propertyType() == flat::ShaderPropertyType_ALBEDO)
					{
						materialParamsPackIndex = i;
						materialTexParamsIndex = j;
						textureName = textureParams->Get(j)->value()->assetName();
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

		const r2::SArray<r2::draw::tex::Texture>* textures =draw::texche::GetTexturesForTexturePack(*mTexturePacksCache, materialPack->pack()->Get(materialParamsPackIndex)->shaderParams()->textureParams()->Get(materialTexParamsIndex)->texturePack()->assetName());

		const auto numTextures = r2::sarr::Size(*textures);

		for (u32 i = 0; i < numTextures; ++i)
		{
			const draw::tex::Texture& texture = r2::sarr::At(*textures, i);
			if (texture.textureAssetHandle.handle == textureName)
			{
				return &texture;
			}
		}

		return nullptr;
	}

	const r2::draw::tex::CubemapTexture* GameAssetManager::GetCubemapTextureForMaterialName(const flat::MaterialPack* materialPack, u64 materialName)
	{
		s32 materialParamsPackIndex = -1;
		s32 materialTexParamsIndex = -1;

		for (flatbuffers::uoffset_t i = 0; i < materialPack->pack()->size(); ++i)
		{
			const flat::Material* material = materialPack->pack()->Get(i);
			if (material->assetName()->assetName() == materialName)
			{
				const auto* textureParams = material->shaderParams()->textureParams();
				const auto numTextureParams = textureParams->size();

				for (u32 j = 0; j < numTextureParams; ++j)
				{
					if (textureParams->Get(j)->propertyType() == flat::ShaderPropertyType_ALBEDO)
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

		return r2::draw::texche::GetCubemapTextureForTexturePack(*mTexturePacksCache, materialPack->pack()->Get(materialParamsPackIndex)->shaderParams()->textureParams()->Get(materialTexParamsIndex)->texturePack()->assetName());
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

#ifdef R2_ASSET_PIPELINE

	bool GameAssetManager::ReloadTextureInTexturePack(u64 texturePackName, u64 textureName)
	{
		return draw::texche::ReloadTextureInTexturePack(*mTexturePacksCache, texturePackName, textureName);
	}

	bool GameAssetManager::ReloadTexturePack(u64 texturePackName)
	{
		return draw::texche::ReloadTexturePack(*mTexturePacksCache, texturePackName);
	}

#endif


}