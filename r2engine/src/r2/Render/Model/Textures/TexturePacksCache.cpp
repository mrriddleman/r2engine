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
	bool UnloadTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
	bool LoadCubemap(TexturePacksCache& texturePacksCache, LoadedTexturePack& loadedTexturePack, const flat::TexturePack* texturePack);
	void UnloadCubemap(TexturePacksCache& texturePacksCache, LoadedTexturePack& loadedTexturePack);

	bool GetTexturePacksCacheSizes(const char* texturePacksManifestPath, u32& numTextures, u32& numTexturePacks, u32& numCubemaps, u32& cacheSize)
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

		const auto texturePacks = texturePacksManifest->texturePacks();

		for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
		{
			if (texturePacks->Get(i)->metaData()->type() == flat::TextureType::TextureType_CUBEMAP)
			{
				numCubemaps ++ ;
			}
		}

		FREE(fileData, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	TexturePacksManifestHandle AddTexturePacksManifestFile(TexturePacksCache& texturePacksCache, u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest)
	{
		//add the texturePacksManifestFilePath file 
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return TexturePacksManifestHandle::Invalid;
		}
		
		if (texturePacksManifest == nullptr || texturePackHandle == 0)
		{
			R2_CHECK(false, "Passed in an invalid texture pack manifest file path!");
			return TexturePacksManifestHandle::Invalid;
		}

		s32 invalidIndex = -1;
		s32 packManifestIndex = r2::shashmap::Get(*texturePacksCache.mManifestNameToTexturePackManifestEntryMap, texturePackHandle, invalidIndex);

		if (invalidIndex != packManifestIndex)
		{
			//we already have this - nothing to do
			return { packManifestIndex };
		}

		TexturePackManifestEntry newEntry;

		newEntry.flatTexturePacksManifest = texturePacksManifest;
		R2_CHECK(newEntry.flatTexturePacksManifest != nullptr, "We should have the flatbuffer data");

		packManifestIndex = r2::sarr::Size(*texturePacksCache.mTexturePackManifests);
		r2::sarr::Push(*texturePacksCache.mTexturePackManifests, newEntry);

		r2::shashmap::Set(*texturePacksCache.mManifestNameToTexturePackManifestEntryMap, texturePackHandle, packManifestIndex);

		const flat::TexturePacksManifest* manifest = newEntry.flatTexturePacksManifest;

		auto texturePacks = manifest->texturePacks();

		for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
		{
			const flat::TexturePack* texturePack = texturePacks->Get(i);

			auto texturePackName = texturePack->packName();

			r2::shashmap::Set(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, packManifestIndex);
		}

		return { packManifestIndex };
	}

	bool UpdateTexturePacksManifest(TexturePacksCache& texturePacksCache, u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return false;
		}

		if (texturePacksManifest == nullptr || texturePackHandle == 0)
		{
			R2_CHECK(false, "Passed in an invalid texture pack manifest file path!");
			return false;
		}

		s32 invalidIndex = -1;
		s32 packManifestIndex = r2::shashmap::Get(*texturePacksCache.mManifestNameToTexturePackManifestEntryMap, texturePackHandle, invalidIndex);

		if (invalidIndex == packManifestIndex)
		{
			//nothing to do in this case
			return true;
		}
		
		TexturePackManifestEntry& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, packManifestIndex);
		entry.flatTexturePacksManifest = texturePacksManifest;

		const flat::TexturePacksManifest* manifest = entry.flatTexturePacksManifest;

		auto texturePacks = manifest->texturePacks();

		for (flatbuffers::uoffset_t i = 0; i < texturePacks->size(); ++i)
		{
			const flat::TexturePack* texturePack = texturePacks->Get(i);

			auto texturePackName = texturePack->packName();

			r2::shashmap::Set(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, packManifestIndex);
		}
		
		auto iter = r2::shashmap::Begin(*texturePacksCache.mLoadedTexturePacks);

		for (; iter != r2::shashmap::End(*texturePacksCache.mLoadedTexturePacks); ++iter)
		{
			s32 manifestIndex = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, iter->value.packName, invalidIndex);

			if (manifestIndex == packManifestIndex)
			{
				for (u32 i = 0; i < manifest->texturePacks()->size(); ++i)
				{
					if (manifest->texturePacks()->Get(i)->packName() == iter->value.packName)
					{
						iter->value.metaData = manifest->texturePacks()->Get(i)->metaData();
						break;
					}
				}
			}
		}

		return true;
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

	void UnloadCubemap(TexturePacksCache& texturePacksCache, LoadedTexturePack& loadedTexturePack)
	{
		for (u32 m = 0; m < loadedTexturePack.cubemap.numMipLevels; ++m)
		{
			const auto mipLevel = loadedTexturePack.metaData->mipLevels()->Get(m);

			const auto numSides = mipLevel->sides()->size();

			for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
			{
				texturePacksCache.mnoptrGameAssetManager->UnloadAsset(loadedTexturePack.cubemap.mips[m].sides[i].textureAssetHandle);

				loadedTexturePack.cubemap.mips[m].sides[i].textureAssetHandle = {};
			}
		}

		loadedTexturePack.cubemap.numMipLevels = 0;
	}

	bool LoadCubemap(TexturePacksCache& texturePacksCache, LoadedTexturePack& loadedTexturePack, const flat::TexturePack* texturePack)
	{
		loadedTexturePack.cubemap.numMipLevels = loadedTexturePack.metaData->mipLevels()->size();

		for (u32 m = 0; m < loadedTexturePack.cubemap.numMipLevels; ++m)
		{
			const auto mipLevel = texturePack->metaData()->mipLevels()->Get(m);

			const auto numSides = mipLevel->sides()->size();

			loadedTexturePack.cubemap.mips[m].mipLevel = mipLevel->level();

			for (flatbuffers::uoffset_t s = 0; s < numSides; ++s)
			{
				const auto side = mipLevel->sides()->Get(s);

				r2::asset::Asset textureAsset = r2::asset::Asset::MakeAssetFromFilePath(side->textureName()->c_str(), r2::asset::CUBEMAP_TEXTURE);

				r2::draw::tex::Texture texture;
				texture.type = tex::TextureType::Diffuse;
				texture.textureAssetHandle = texturePacksCache.mnoptrGameAssetManager->LoadAsset(textureAsset);

				R2_CHECK(!r2::asset::IsInvalidAssetHandle(texture.textureAssetHandle), "We couldn't load the texture: %s!", side->textureName()->c_str());

				loadedTexturePack.cubemap.mips[m].sides[s] = texture;
			}
		}

		return true;
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
			LoadCubemap(texturePacksCache, loadedTexturePack, texturePack);
		}

		r2::shashmap::Set(*texturePacksCache.mLoadedTexturePacks, loadedTexturePack.packName, loadedTexturePack);

		return true;
	}

	bool LoadAllTexturePacks(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
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

	bool LoadTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		s32 invalidIndex = -1;

		s32 index = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, invalidIndex);

		if (invalidIndex == index)
		{
			return false;
		}

		return LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, { index }, texturePackName);
	}

	bool LoadTexturePacks(TexturePacksCache& texturePacksCache, const r2::SArray<u64>& texturePacks)
	{
		const auto numTexturePacks = r2::sarr::Size(texturePacks);

		for (u32 i = 0; i < numTexturePacks; ++i)
		{
			bool result = LoadTexturePack(texturePacksCache, r2::sarr::At(texturePacks, i));

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

	bool UnloadAllTexturePacks(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
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
			UnloadTexturePack(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->packName());
		}

		return true;
	}

	bool UnloadTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		s32 invalidIndex = -1;

		s32 index = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, invalidIndex);

		if (invalidIndex == index)
		{
			return false;
		}

		return UnloadTexturePack(texturePacksCache, { index }, texturePackName);
	}

	bool UnloadTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
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

			if (resultTexturePack.packName == defaultLoadedTexturePack.packName )
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
				UnloadCubemap(texturePacksCache, resultTexturePack);
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

	bool ReloadTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		bool result = UnloadTexturePack(texturePacksCache, texturePackName);

		R2_CHECK(result, "Failed to unload the texture pack?");

		return LoadTexturePack(texturePacksCache, texturePackName);
	}

	r2::draw::tex::TextureType GetTextureTypeForTextureName(const flat::TexturePack* texturePack, u64 textureName)
	{
		r2::draw::tex::TextureType textureType = tex::NUM_TEXTURE_TYPES;

		const auto* albedos = texturePack->albedo();

		for (flatbuffers::uoffset_t i = 0; i < albedos->size(); ++i)
		{

			std::string albedoString = albedos->Get(i)->c_str();
			if (r2::asset::Asset::GetAssetNameForFilePath(albedoString.c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Diffuse;
			}
		}

		const auto* metallics = texturePack->metallic();

		for (flatbuffers::uoffset_t i = 0; i < metallics->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(metallics->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Metallic;
			}
		}

		const auto* roughnesses = texturePack->roughness();

		for (flatbuffers::uoffset_t i = 0; i < roughnesses->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(roughnesses->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Roughness;
			}
		}

		const auto* normals = texturePack->normal();

		for (flatbuffers::uoffset_t i = 0; i < normals->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(normals->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Normal;
			}
		}

		const auto* aos = texturePack->occlusion();

		for (flatbuffers::uoffset_t i = 0; i < aos->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(aos->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Occlusion;
			}
		}

		const auto* emissives = texturePack->emissive();

		for (flatbuffers::uoffset_t i = 0; i < emissives->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(emissives->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Emissive;
			}
		}

		const auto* anisotropies = texturePack->anisotropy();

		for (flatbuffers::uoffset_t i = 0; i < anisotropies->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(anisotropies->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Anisotropy;
			}
		}

		const auto* clearCoats = texturePack->clearCoat();

		for (flatbuffers::uoffset_t i = 0; i < clearCoats->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(clearCoats->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::ClearCoat;
			}
		}

		const auto* clearCoatNormals = texturePack->clearCoatNormal();

		for (flatbuffers::uoffset_t i = 0; i < clearCoatNormals->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(clearCoatNormals->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::ClearCoatNormal;
			}
		}

		const auto* clearCoatRoughnesses = texturePack->clearCoatRoughness();

		for (flatbuffers::uoffset_t i = 0; i < clearCoatRoughnesses->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(clearCoatRoughnesses->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::ClearCoatRoughness;
			}
		}

		const auto* details = texturePack->detail();

		for (flatbuffers::uoffset_t i = 0; i < details->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(details->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Detail;
			}
		}

		const auto* heights = texturePack->height();

		for (flatbuffers::uoffset_t i = 0; i < heights->size(); ++i)
		{
			if (r2::asset::Asset::GetAssetNameForFilePath(heights->Get(i)->c_str(), r2::asset::TEXTURE) == textureName)
			{
				return tex::TextureType::Height;
			}
		}

		return tex::TextureType::NUM_TEXTURE_TYPES;
	}

	bool ReloadTextureInTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName, u64 textureName)
	{
		LoadedTexturePack defaultTexturePack;
		LoadedTexturePack& loadedTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultTexturePack);

		if (loadedTexturePack.packName == defaultTexturePack.packName)
		{
			//we never loaded this pack, load it
			return LoadTexturePack(texturePacksCache, texturePackName);
		}

		//figure out which one to load
		//first get the manifest data
		s32 invalidIndex = -1;
		s32 index = r2::shashmap::Get(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, invalidIndex);

		R2_CHECK(index != invalidIndex, "Should never happen");

		const TexturePackManifestEntry& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, index);

		//now find the pack
		const auto* texturePacks = entry.flatTexturePacksManifest->texturePacks();
		const auto numTexturePacks = texturePacks->size();
		const flat::TexturePack* foundPack = nullptr;
		for (flatbuffers::uoffset_t i = 0; i < numTexturePacks; ++i)
		{
			if (texturePacks->Get(i)->packName() == texturePackName)
			{
				//find the texture in question
				foundPack = texturePacks->Get(i);
				break;
			}
		}

		if (!foundPack)
		{
			R2_CHECK(false, "Dunno how this would ever happen");
			return false;
		}

		tex::TextureType textureType;
		if (foundPack->metaData()->type() == flat::TextureType_TEXTURE)
		{
			//get the type in case we have never loaded this texture
			textureType = GetTextureTypeForTextureName(foundPack, textureName);

			//find the texture in the loadedTexturePack
			const auto numTextures = r2::sarr::Size(*loadedTexturePack.textures);
			const tex::Texture* foundTexture = nullptr;
			s32 foundIndex = -1;
			for (u32 i = 0; i < numTextures; ++i)
			{
				const auto& texture = r2::sarr::At(*loadedTexturePack.textures, i);

				if (texture.textureAssetHandle.handle == textureName && texture.type == textureType)
				{
					foundTexture = &texture;
					foundIndex = i;
					break;
				}
			}

			r2::asset::Asset textureAsset = r2::asset::Asset(textureName, r2::asset::TEXTURE);

			if (textureType != tex::TextureType::NUM_TEXTURE_TYPES && foundTexture) 
			{
				//we have the texture and it's in the manifest - just reload
				auto handle = texturePacksCache.mnoptrGameAssetManager->ReloadAsset(textureAsset);

				R2_CHECK(!r2::asset::IsInvalidAssetHandle(handle), "We couldn't reload the asset");
			}
			else if (textureType != tex::TextureType::NUM_TEXTURE_TYPES && !foundTexture)
			{
				//we have the texture in the manifest, not loaded - added new texture
				tex::Texture newTexture;
				newTexture.type = textureType;

				newTexture.textureAssetHandle = texturePacksCache.mnoptrGameAssetManager->LoadAsset(textureAsset);

				R2_CHECK(!r2::asset::IsInvalidAssetHandle(newTexture.textureAssetHandle), "We couldn't load the asset");

				r2::sarr::Push(*loadedTexturePack.textures, newTexture);
			}
			else if (textureType == tex::TextureType::NUM_TEXTURE_TYPES && foundTexture)
			{
				//we have the texture but it's not in the manifest - removed the texture
				texturePacksCache.mnoptrGameAssetManager->UnloadAsset(foundTexture->textureAssetHandle);

				r2::sarr::RemoveAndSwapWithLastElement(*loadedTexturePack.textures, index);
			}
			else
			{
				//nothing? - maybe the case where we never loaded it and removed it
			}
		}
		else
		{
			bool foundInPack = false;
			for (flatbuffers::uoffset_t i = 0; i < foundPack->albedo()->size(); ++i)
			{
				if (STRING_ID(foundPack->albedo()->Get(i)->c_str()) == textureName)
				{
					foundInPack = true;
					break;
				}
			}

			if (foundInPack && loadedTexturePack.cubemap.numMipLevels > 0)
			{
				//reload it 
				UnloadCubemap(texturePacksCache, loadedTexturePack);
				LoadCubemap(texturePacksCache, loadedTexturePack, foundPack);
			}
			else if(foundInPack && loadedTexturePack.cubemap.numMipLevels == 0)
			{
				//add it
				LoadCubemap(texturePacksCache, loadedTexturePack, foundPack);
			}
			else if (!foundInPack && loadedTexturePack.cubemap.numMipLevels > 0)
			{
				//removed
				UnloadCubemap(texturePacksCache, loadedTexturePack);
			}
			else
			{
				//nothing? - maybe the case where we never loaded it and removed it
			}
		}

		return true;
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
			bool loaded = LoadTexturePack(texturePacksCache, texturePackName);

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

	const tex::Texture* GetTextureFromTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName, u64 textureName)
	{
		if (texturePacksCache.mnoptrGameAssetManager == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
			return nullptr;
		}

		LoadedTexturePack defaultLoadedTexturePack;

		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
		LoadedTexturePack* loadedTexturePack = &resultTexturePack;

		if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
		{
			bool loaded = LoadTexturePack(texturePacksCache, texturePackName);

			if (!loaded)
			{
				R2_CHECK(false, "We couldn't seem to load the texture pack");
				return nullptr;
			}

			LoadedTexturePack& foundTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);

			if (foundTexturePack.metaData->type() == flat::TextureType_CUBEMAP)
			{
				R2_CHECK(false, "Wrong type, this is a cubemap");
				return nullptr;
			}

			loadedTexturePack = &foundTexturePack;
		}

		const auto numTextures = r2::sarr::Size(*loadedTexturePack->textures);

		for (u32 i = 0; i < numTextures; ++i)
		{
			const tex::Texture& texture = r2::sarr::At(*loadedTexturePack->textures, i);
			if (texture.textureAssetHandle.handle == textureName)
			{
				return &texture;
			}
		}

		return nullptr;
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
			bool loaded = LoadTexturePack(texturePacksCache, texturePackName);

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