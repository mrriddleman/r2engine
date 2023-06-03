#include "r2pch.h"

#include "r2/Render/Model/Textures/TexturePacksCache.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "r2/Render/Model/Textures/TexturePackMetaData_generated.h"
#include "r2/Utils/Hash.h"
#include <algorithm>

namespace r2::draw
{
	u64 TexturePacksCache::MemorySize(u32 numTextures, u32 numManifests, u32 numTexturePacks)
	{
		u64 memorySize = 0;

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u32 freeListHeaderSize = r2::mem::FreeListAllocator::HeaderSize();

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		const u32 ALIGNMENT = 16;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TexturePacksCache), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::FreeListArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<TexturePackManifestEntry>::MemorySize(numManifests), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<LoadedTexturePack>::MemorySize(numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::SArray<tex::Texture>), ALIGNMENT, freeListHeaderSize, boundsChecking) * numTexturePacks;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(tex::Texture), ALIGNMENT, freeListHeaderSize, boundsChecking) * numTextures;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s32>::MemorySize(numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s32>::MemorySize(numManifests * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);

		return memorySize;
	}
}

namespace r2::draw::texche 
{

	const TexturePacksManifestHandle TexturePacksManifestHandle::Invalid = { -1 };

	const u32 ALIGNMENT = 16;
	void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList);
	bool LoadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
	bool UnloadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);

	bool GetTexturePacksCacheSizes(const char* texturePacksManifestPath, u32& numTextures, u32& numTexturePacks, u32& cacheSize)
	{
		if (!texturePacksManifestPath)
		{
			return false;
		}

		R2_CHECK(strlen(texturePacksManifestPath) > 0, "We shouldn't have an improper path here");

		u64 fileSize = 0;
		void* fileData = r2::fs::ReadFile<r2::mem::StackArena>(*MEM_ENG_SCRATCH_PTR, texturePacksManifestPath, fileSize);

		if (!fileData)
		{
			R2_CHECK(false, "Unable to read the data from: %s\n", texturePacksManifestPath);
			return false;
		}

		const flat::TexturePacksManifest* texturePacksManifest = flat::GetTexturePacksManifest(fileData);

		R2_CHECK(texturePacksManifest != nullptr, "Somehow we couldn't get the proper flat::TexturePacksManifest data!");

		cacheSize = static_cast<u32>(texturePacksManifest->totalTextureSize()) + static_cast<u32>(fileSize); //+ the file size if we want to keep that around
		numTextures = static_cast<u32>(texturePacksManifest->totalNumberOfTextures());
		numTexturePacks = static_cast<u32>(texturePacksManifest->texturePacks()->size());
		
		FREE(fileData, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	TexturePacksManifestHandle AddTexturePacksManifestFile(TexturePacksCache& texturePacksCache, const char* texturePacksManifestFilePath)
	{
		//add the texturePacksManifestFilePath file 
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return TexturePacksManifestHandle::Invalid;
		}
		
		if (texturePacksManifestFilePath == nullptr || strlen(texturePacksManifestFilePath) == 0)
		{
			R2_CHECK(false, "Passed in an invalid texture pack manifest file path!");
			return TexturePacksManifestHandle::Invalid;
		}

		r2::asset::Asset manifestAsset = r2::asset::Asset::MakeAssetFromFilePath(texturePacksManifestFilePath, r2::asset::TEXTURE_PACK_MANIFEST);

		s32 invalidIndex = -1;
		s32 packManifestIndex = r2::shashmap::Get(*texturePacksCache.mManifestNameToTexturePackManifestEntryMap, manifestAsset.HashID(), invalidIndex);

		if (invalidIndex != packManifestIndex)
		{
			//we already have this - nothing to do
			return { packManifestIndex };
		}

		r2::asset::FileList fileList = texturePacksCache.mnoptrGameAssetManager->GetFileList();

		r2::asset::RawAssetFile* texturePacksManifestFile = r2::asset::lib::MakeRawAssetFile(texturePacksManifestFilePath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::TEXTURE_PACK_MANIFEST));

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)texturePacksManifestFile);

		TexturePackManifestEntry newEntry;

		newEntry.flatTexturePacksManifest = flat::GetTexturePacksManifest(texturePacksCache.mnoptrGameAssetManager->LoadAndGetAssetConst<byte>(manifestAsset));
		R2_CHECK(newEntry.flatTexturePacksManifest != nullptr, "We should have the flatbuffer data");

		const u32 numTexturePackManifests = r2::sarr::Capacity(*texturePacksCache.mTexturePackManifests);

		packManifestIndex = r2::sarr::Size(*texturePacksCache.mTexturePackManifests);
		r2::sarr::Push(*texturePacksCache.mTexturePackManifests, newEntry);

		r2::shashmap::Set(*texturePacksCache.mManifestNameToTexturePackManifestEntryMap, manifestAsset.HashID(), packManifestIndex);

		const flat::TexturePacksManifest* manifest = newEntry.flatTexturePacksManifest;


		auto texturePacks = manifest->texturePacks();

		for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
		{
			const flat::TexturePack* texturePack = texturePacks->Get(i);

			AddAllTexturePathsInTexturePackToFileList(texturePack, fileList);

			r2::shashmap::Set(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePack->packName(), packManifestIndex);
		}

		return { packManifestIndex };
	}

	void LoadTextures(
		TexturePacksCache& texturePacksCache,
		LoadedTexturePack& loadedTexturePack,
		const flat::TexturePack* texturePack,
		const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* paths,
		r2::draw::tex::TextureType type)
	{
		for (u32 i = 0; i < paths->size(); ++i)
		{
			const flatbuffers::String* texturePath = paths->Get(i);

			r2::asset::Asset textureAsset = r2::asset::Asset::MakeAssetFromFilePath(texturePath->c_str(), r2::asset::TEXTURE);

			tex::Texture newTexture;
			newTexture.type = type;

			newTexture.textureAssetHandle = texturePacksCache.mnoptrGameAssetManager->LoadAsset(textureAsset);

			R2_CHECK(!r2::asset::IsInvalidAssetHandle(newTexture.textureAssetHandle), "We couldn't load the texture: %s!", texturePath->c_str());

			r2::sarr::Push(*loadedTexturePack.textures, newTexture);
		}
	}

	bool LoadAllTexturesFromTexturePackFromDisk(TexturePacksCache& texturePacksCache, const flat::TexturePack* texturePack)
	{
		if (texturePack == nullptr)
		{
			return false;
		}

		const auto numTexturesInThePack = texturePack->totalNumberOfTextures();

		LoadedTexturePack loadedTexturePack;
		
		loadedTexturePack.packName = texturePack->packName();
		loadedTexturePack.metaData = texturePack->metaData();

		if (loadedTexturePack.metaData->type() == flat::TextureType_TEXTURE)
		{
			loadedTexturePack.textures = MAKE_SARRAY(*texturePacksCache.mTexturePackArena, tex::Texture, numTexturesInThePack);
			
			//now load all of the textures of this pack
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->albedo(), tex::TextureType::Diffuse);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->anisotropy(), tex::TextureType::Anisotropy);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->clearCoat(), tex::TextureType::ClearCoat);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->clearCoatNormal(), tex::TextureType::ClearCoatNormal);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->clearCoatRoughness(), tex::TextureType::ClearCoatRoughness);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->detail(), tex::TextureType::Detail);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->emissive(), tex::TextureType::Emissive);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->height(), tex::TextureType::Height);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->metallic(), tex::TextureType::Metallic);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->normal(), tex::TextureType::Normal);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->occlusion(), tex::TextureType::Occlusion);
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->roughness(), tex::TextureType::Roughness);
		}
		else if (loadedTexturePack.metaData->type() == flat::TextureType_CUBEMAP)
		{
			loadedTexturePack.cubemap.numMipLevels = loadedTexturePack.metaData->mipLevels()->size();

			for (u32 m = 0; m < loadedTexturePack.cubemap.numMipLevels; ++m)
			{
				const auto mipLevel = texturePack->metaData()->mipLevels()->Get(m);

				const auto numSides = mipLevel->sides()->size();

				loadedTexturePack.cubemap.mips[m].mipLevel = mipLevel->level();

				for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
				{
					const auto side = mipLevel->sides()->Get(i);

					r2::asset::Asset textureAsset = r2::asset::Asset::MakeAssetFromFilePath(side->textureName()->c_str(), r2::asset::CUBEMAP_TEXTURE);

					r2::draw::tex::Texture texture;
					texture.type = tex::TextureType::Diffuse;

					loadedTexturePack.cubemap.mips[m].sides[i] = texture;
				}
			}
		}

		r2::shashmap::Set(*texturePacksCache.mLoadedTexturePacks, loadedTexturePack.packName, loadedTexturePack);

		return true;
	}

	bool LoadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
	{
		if (texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return false;
		}

		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
		{
			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
			return false;
		}

		const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);

		const flat::TexturePacksManifest* texturePacksManifest = entry.flatTexturePacksManifest;
		
		for (flatbuffers::uoffset_t i = 0; i < texturePacksManifest->texturePacks()->size(); ++i)
		{
			//@NOTE(Serge): we want to use this function since it checks to see if we have it loaded already
			LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->packName());
		}

		return true;
	}

	bool LoadTexturePackFromDisk(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		s32 invalidIndex = -1;

		s32 index = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, invalidIndex);

		if (invalidIndex == index)
		{
			return false;
		}

		return LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, { index }, texturePackName);
	}

	bool LoadTexturePacksFromDisk(TexturePacksCache& texturePacksCache, const r2::SArray<u64>& texturePacks)
	{
		const auto numTexturePacks = r2::sarr::Size(texturePacks);

		for (u32 i = 0; i < numTexturePacks; ++i)
		{
			bool result = LoadTexturePackFromDisk(texturePacksCache, r2::sarr::At(texturePacks, i));

			R2_CHECK(result != false, "?");
		}

		return true;
	}

	bool LoadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
	{
		if (texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return false;
		}

		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
		{
			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
			return false;
		}

		//first check to see if we have it already
		LoadedTexturePack defaultLoadedTexturePack;

		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

		if (resultTexturePack.packName != defaultLoadedTexturePack.packName)
		{
			return true;
		}

		const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);

		const flat::TexturePacksManifest* texturePacksManifest = entry.flatTexturePacksManifest;

		//find the texture pack
		const flat::TexturePack* texturePack = nullptr;
		for (flatbuffers::uoffset_t i = 0; i < texturePacksManifest->texturePacks()->size(); ++i)
		{
			if (texturePacksManifest->texturePacks()->Get(i)->packName() == texturePackName)
			{
				texturePack = texturePacksManifest->texturePacks()->Get(i);
				break;
			}
		}

		if (texturePack == nullptr)
		{
			R2_CHECK(false, "We couldn't find the texturePack for packName: %llu", texturePackName);
			return false;
		}

		return LoadAllTexturesFromTexturePackFromDisk(texturePacksCache, texturePack);
	}

	bool UnloadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return false;
		}

		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
		{
			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
			return false;
		}

		const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);

		const flat::TexturePacksManifest* texturePacksManifest = entry.flatTexturePacksManifest;

		for (flatbuffers::uoffset_t i = 0; i < texturePacksManifest->texturePacks()->size(); ++i)
		{
			//@NOTE(Serge): we want to use this function since it checks to see if we have it loaded already
			UnloadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->packName());
		}

		return true;
	}

	bool UnloadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		s32 invalidIndex = -1;

		s32 index = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, invalidIndex);

		if (invalidIndex == index)
		{
			return false;
		}

		return UnloadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, { index }, texturePackName);
	}

	bool UnloadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return false;
		}

		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
		{
			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
			return false;
		}

		//first check to see if we even loaded the pack
		{
			LoadedTexturePack defaultLoadedTexturePack;
			
			LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

			if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
			{
				//we don't have it loaded so nothing to do!
				return true;
			}

			//we did find the loaded files - now unload them
			if (resultTexturePack.metaData->type() == flat::TextureType_TEXTURE)
			{
				const auto numTextures = r2::sarr::Size(*resultTexturePack.textures);

				for (u32 i = 0; i < numTextures; ++i)
				{
					const tex::Texture& texture = r2::sarr::At(*resultTexturePack.textures, i);

					texturePacksCache.mnoptrGameAssetManager->UnloadAsset(texture.textureAssetHandle);
				}

				FREE(resultTexturePack.textures, *texturePacksCache.mTexturePackArena);
			}
			else
			{
				for (u32 m = 0; m < resultTexturePack.cubemap.numMipLevels; ++m)
				{
					const auto mipLevel = resultTexturePack.metaData->mipLevels()->Get(m);

					const auto numSides = mipLevel->sides()->size();

					for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
					{
						texturePacksCache.mnoptrGameAssetManager->UnloadAsset(resultTexturePack.cubemap.mips[m].sides[i].textureAssetHandle);
					}
				}
			}
		}
		
		r2::shashmap::Remove(*texturePacksCache.mLoadedTexturePacks, texturePackName);

		return true;
	}

	void AddAllTexturesFromTextureType(const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* texturePaths, r2::asset::FileList fileList)
	{
		for (u32 i = 0; i < texturePaths->size(); ++i)
		{
			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(texturePaths->Get(i)->c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::TEXTURE));
			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
		}
	}

	void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList)
	{
		AddAllTexturesFromTextureType(texturePack->albedo(), fileList);
		AddAllTexturesFromTextureType(texturePack->anisotropy(), fileList);
		AddAllTexturesFromTextureType(texturePack->clearCoat(), fileList);
		AddAllTexturesFromTextureType(texturePack->clearCoatNormal(), fileList);
		AddAllTexturesFromTextureType(texturePack->clearCoatRoughness(), fileList);
		AddAllTexturesFromTextureType(texturePack->detail(), fileList);
		AddAllTexturesFromTextureType(texturePack->emissive(), fileList);
		AddAllTexturesFromTextureType(texturePack->height(), fileList);
		AddAllTexturesFromTextureType(texturePack->metallic(), fileList);
		AddAllTexturesFromTextureType(texturePack->normal(), fileList);
		AddAllTexturesFromTextureType(texturePack->occlusion(), fileList);
		AddAllTexturesFromTextureType(texturePack->roughness(), fileList);

		if (texturePack->metaData() && texturePack->metaData()->mipLevels())
		{
			for (flatbuffers::uoffset_t i = 0; i < texturePack->metaData()->mipLevels()->size(); ++i)
			{
				const flat::MipLevel* mipLevel = texturePack->metaData()->mipLevels()->Get(i);

				for (flatbuffers::uoffset_t side = 0; side < mipLevel->sides()->size(); ++side)
				{
					r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(mipLevel->sides()->Get(side)->textureName()->c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::CUBEMAP_TEXTURE));
					r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
				}
			}
		}
	}

	bool IsTexturePackACubemap(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return false;
		}

		s32 invalidIndex = -1;
		s32 manifestIndex = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, invalidIndex);

		if (manifestIndex == invalidIndex)
		{
			R2_CHECK(false, "We couldn't find the texture pack!");
			return false;
		}

		const TexturePackManifestEntry& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, manifestIndex);

		const auto numTexturePacks = entry.flatTexturePacksManifest->texturePacks()->size();

		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			if (entry.flatTexturePacksManifest->texturePacks()->Get(i)->packName() == texturePackName)
			{
				return entry.flatTexturePacksManifest->texturePacks()->Get(i)->metaData()->type() == flat::TextureType::TextureType_CUBEMAP;
			}
		}

		R2_CHECK(false, "Should never get here");
		return false;
	}

	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return nullptr;
		}

		LoadedTexturePack defaultLoadedTexturePack;

		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

		if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
		{
			bool loaded = LoadTexturePackFromDisk(texturePacksCache, texturePackName);

			if (!loaded)
			{
				R2_CHECK(false, "We couldn't seem to load the texture pack");
				return nullptr;
			}

			LoadedTexturePack& loadedTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

			//we haven't loaded the texture pack yet
			return loadedTexturePack.textures;
		}

		return resultTexturePack.textures;
	}

	const tex::CubemapTexture* GetCubemapTextureForTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return nullptr;
		}

		LoadedTexturePack defaultLoadedTexturePack;

		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

		if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
		{
			bool loaded = LoadTexturePackFromDisk(texturePacksCache, texturePackName);

			if (!loaded)
			{
				R2_CHECK(false, "We couldn't seem to load the texture pack");
				return nullptr;
			}

			LoadedTexturePack& loadedTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

			//we haven't loaded the texture pack yet
			return &loadedTexturePack.cubemap;
		}

		return &resultTexturePack.cubemap;
	}

}