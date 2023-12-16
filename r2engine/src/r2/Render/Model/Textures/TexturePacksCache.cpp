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
#include "R2/Render/Model/Materials/MaterialPack_generated.h"
#include "r2/Core/Assets/AssetRef_generated.h"
#include "r2/Utils/Hash.h"
#include <algorithm>

namespace
{
	const u64 EMPTY_TEXTURE_PACK_NAME = STRING_ID("");
}

namespace r2::draw
{
	u64 TexturePacksCache::MemorySize(u32 textureCapacity, u32 numTextures, u32 numManifests, u32 numTexturePacks)
	{
		u64 memorySize = 0;

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u32 freeListHeaderSize = r2::mem::FreeListAllocator::HeaderSize();

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		const u32 ALIGNMENT = 16;

		/*
				r2::mem::StackArena* mArena;

		r2::SArray<TexturePackManifestEntry>* mTexturePackManifests;
		r2::SHashMap<LoadedTexturePack>* mLoadedTexturePacks;
		r2::SHashMap<s32>* mPackNameToTexturePackManifestEntryMap;
		r2::SHashMap<s32>* mManifestNameToTexturePackManifestEntryMap;

		r2::mem::FreeListArena* mTexturePackArena;

		r2::asset::AssetCache* mAssetCache;
		*/

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TexturePacksCache), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::FreeListArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<TexturePackManifestEntry>::MemorySize(numManifests), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<LoadedTexturePack>::MemorySize(numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::SArray<tex::Texture>), ALIGNMENT, freeListHeaderSize, boundsChecking) * numTexturePacks;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(tex::Texture), ALIGNMENT, freeListHeaderSize, boundsChecking) * numTextures;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s32>::MemorySize(numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s32>::MemorySize(numManifests * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::asset::AssetCache::TotalMemoryNeeded(numTextures, textureCapacity, ALIGNMENT, numTextures, numTextures), ALIGNMENT, stackHeaderSize, boundsChecking);
		
		return memorySize;
	}
}

namespace r2::draw::texche 
{

	const TexturePacksManifestHandle TexturePacksManifestHandle::Invalid = { -1 };

	const u32 ALIGNMENT = 16;

	bool LoadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
	bool UnloadTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
	bool LoadCubemap(TexturePacksCache& texturePacksCache, LoadedTexturePack& loadedTexturePack, const flat::TexturePack* texturePack);
	void UnloadCubemap(TexturePacksCache& texturePacksCache, LoadedTexturePack& loadedTexturePack);

	r2::draw::TexturePacksCache* Create(r2::mem::utils::MemBoundary memBoundary, u32 textureCapacity, u32 numTextures, u32 numManifests, u32 numTexturePacks)
	{
		r2::mem::StackArena* texturePacksCacheArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(memBoundary);

		R2_CHECK(texturePacksCacheArena != nullptr, "We couldn't emplace the stack arena - no way to recover!");

		TexturePacksCache* newTexturePacksCache = ALLOC(TexturePacksCache, *texturePacksCacheArena);
		
		R2_CHECK(newTexturePacksCache != nullptr, "We couldn't create the TexturePacksCache object");

		newTexturePacksCache->mMemoryBoundary = memBoundary;

		newTexturePacksCache->mArena = texturePacksCacheArena;

		newTexturePacksCache->mTexturePackManifests = MAKE_SARRAY(*texturePacksCacheArena, TexturePackManifestEntry, numManifests);

		R2_CHECK(newTexturePacksCache->mTexturePackManifests != nullptr, "We couldn't create the texture pack manifests array");

		newTexturePacksCache->mLoadedTexturePacks = MAKE_SHASHMAP(*texturePacksCacheArena, LoadedTexturePack, numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier());

		R2_CHECK(newTexturePacksCache->mLoadedTexturePacks != nullptr, "We couldn't create the mLoadedTexturePacks hashmap");

		newTexturePacksCache->mPackNameToTexturePackManifestEntryMap = MAKE_SHASHMAP(*texturePacksCacheArena, s32, numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier());

		R2_CHECK(newTexturePacksCache->mPackNameToTexturePackManifestEntryMap != nullptr, "We couldn't create the mPackNameToTexturePackManifestEntryMap");

		newTexturePacksCache->mManifestNameToTexturePackManifestEntryMap = MAKE_SHASHMAP(*texturePacksCacheArena, s32, numManifests * r2::SHashMap<u32>::LoadFactorMultiplier());

		R2_CHECK(newTexturePacksCache->mManifestNameToTexturePackManifestEntryMap != nullptr, "We couldn't create the mManifestNameToTexturePackManifestEntryMap");

		//first we need to calculate how big this arena is
		u32 freelistArenaSize = sizeof(r2::SArray<tex::Texture>) * numTexturePacks + sizeof(tex::Texture) * numTextures;

		newTexturePacksCache->mTexturePackArena = MAKE_FREELIST_ARENA(*texturePacksCacheArena, freelistArenaSize, r2::mem::FIND_BEST);

		R2_CHECK(newTexturePacksCache->mTexturePackArena != nullptr, "We couldn't create the the texture pack arena");

		const auto memoryNeeded = r2::asset::AssetCache::TotalMemoryNeeded(numTextures, textureCapacity, memBoundary.alignment);

		auto boundary = MAKE_MEMORY_BOUNDARY_VERBOSE(*texturePacksCacheArena, memoryNeeded, ALIGNMENT, "TexturePack's AssetCache");
		newTexturePacksCache->mAssetCacheBoundary = boundary;

		newTexturePacksCache->mAssetCache = r2::asset::lib::CreateAssetCache(boundary, textureCapacity, numTextures);

		return newTexturePacksCache;
	}

	void Shutdown(TexturePacksCache* texturePacksCache)
	{
		if (!texturePacksCache)
		{
			return;
		}

		r2::mem::utils::MemBoundary texturePacksCacheBoundary = texturePacksCache->mMemoryBoundary;

		r2::mem::StackArena* texturePacksCacheArena = texturePacksCache->mArena;

		const u32 numTexturePackManifests = r2::sarr::Size(*texturePacksCache->mTexturePackManifests);

		for (u32 i = 0; i < numTexturePackManifests; ++i)
		{
			const auto& entry = r2::sarr::At(*texturePacksCache->mTexturePackManifests, i);

			if (entry.flatTexturePacksManifest != nullptr)
			{
				UnloadAllTexturePacks(*texturePacksCache, { static_cast<s32>(i) });
			}
		}

		r2::sarr::Clear(*texturePacksCache->mTexturePackManifests);
		r2::shashmap::Clear(*texturePacksCache->mLoadedTexturePacks);
		r2::shashmap::Clear(*texturePacksCache->mPackNameToTexturePackManifestEntryMap);
		r2::shashmap::Clear(*texturePacksCache->mManifestNameToTexturePackManifestEntryMap);


		r2::asset::lib::DestroyCache(texturePacksCache->mAssetCache);
		FREE(texturePacksCache->mAssetCacheBoundary.location, *texturePacksCacheArena);


		FREE(texturePacksCache->mTexturePackArena, *texturePacksCacheArena);

		FREE(texturePacksCache->mManifestNameToTexturePackManifestEntryMap, *texturePacksCacheArena);

		FREE(texturePacksCache->mPackNameToTexturePackManifestEntryMap, *texturePacksCacheArena);

		FREE(texturePacksCache->mLoadedTexturePacks, *texturePacksCacheArena);

		FREE(texturePacksCache->mTexturePackManifests, *texturePacksCacheArena);

		FREE(texturePacksCache, *texturePacksCacheArena);

		FREE_EMPLACED_ARENA(texturePacksCacheArena);
	}

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
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
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

			auto texturePackName = texturePack->assetName()->assetName();

			r2::shashmap::Set(*texturePacksCache.mPackNameToTexturePackManifestEntryMap, texturePackName, packManifestIndex);
		}

		return { packManifestIndex };
	}

	bool UpdateTexturePacksManifest(TexturePacksCache& texturePacksCache, u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest)
	{
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
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

			auto texturePackName = texturePack->assetName()->assetName();

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
					if (manifest->texturePacks()->Get(i)->assetName()->assetName() == iter->value.packName)
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
		const flatbuffers::Vector<flatbuffers::Offset<flat::AssetRef>>* paths)
	{
		for (u32 i = 0; i < paths->size(); ++i)
		{
			const flatbuffers::String* texturePath = paths->Get(i)->binPath();

			r2::asset::Asset textureAsset = r2::asset::Asset::MakeAssetFromFilePath(texturePath->c_str(), r2::asset::TEXTURE);

			tex::Texture newTexture;

			newTexture.textureAssetHandle = texturePacksCache.mAssetCache->LoadAsset(textureAsset);

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
				texturePacksCache.mAssetCache->FreeAsset(loadedTexturePack.cubemap.mips[m].sides[i].textureAssetHandle);

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

				texture.textureAssetHandle = texturePacksCache.mAssetCache->LoadAsset(textureAsset);

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
		
		loadedTexturePack.packName = texturePack->assetName()->assetName();
		loadedTexturePack.metaData = texturePack->metaData();

		if (loadedTexturePack.metaData->type() == flat::TextureType_TEXTURE)
		{
			loadedTexturePack.textures = MAKE_SARRAY(*texturePacksCache.mTexturePackArena, tex::Texture, numTexturesInThePack);
			
			//now load all of the textures of this pack
			LoadTextures(texturePacksCache, loadedTexturePack, texturePack, texturePack->textures());
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
			LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->assetName()->assetName());
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
			auto texturePack = r2::sarr::At(texturePacks, i);

			bool result = LoadTexturePack(texturePacksCache, texturePack);

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
			if (texturePacksManifest->texturePacks()->Get(i)->assetName()->assetName() == texturePackName)
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
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
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
			UnloadTexturePack(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->assetName()->assetName());
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

	bool UnloadTexture(TexturePacksCache& texturePacksCache, const tex::Texture& texture)
	{
		texturePacksCache.mAssetCache->FreeAsset(texture.textureAssetHandle);
		return true;
	}

	bool UnloadTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
	{
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
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

					texturePacksCache.mAssetCache->FreeAsset(texture.textureAssetHandle);
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

	bool IsTexturePackACubemap(TexturePacksCache& texturePacksCache, u64 texturePackName)
	{
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
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
			if (entry.flatTexturePacksManifest->texturePacks()->Get(i)->assetName()->assetName() == texturePackName)
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

	bool IsTextureInFlatTexturePack(const flat::TexturePack* texturePack, u64 textureName)
	{
		bool found = false;

		const auto* textures = texturePack->textures();
		for (flatbuffers::uoffset_t i = 0; i < textures->size(); ++i)
		{
			if (textures->Get(i)->assetName()->assetName() == textureName)
			{
				found = true;
				break;
			}
		}

		return found;
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
			if (texturePacks->Get(i)->assetName()->assetName() == texturePackName)
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

		if (foundPack->metaData()->type() == flat::TextureType_TEXTURE)
		{
			//get the type in case we have never loaded this texture
			bool foundInFlatTexturePack = IsTextureInFlatTexturePack(foundPack, textureName);

			//find the texture in the loadedTexturePack
			const auto numTextures = r2::sarr::Size(*loadedTexturePack.textures);
			const tex::Texture* foundTexture = nullptr;
			s32 foundIndex = -1;
			for (u32 i = 0; i < numTextures; ++i)
			{
				const auto& texture = r2::sarr::At(*loadedTexturePack.textures, i);

				if (texture.textureAssetHandle.handle == textureName)
				{
					foundTexture = &texture;
					foundIndex = i;
					break;
				}
			}

			r2::asset::Asset textureAsset = r2::asset::Asset(textureName, r2::asset::TEXTURE);

			if (foundInFlatTexturePack && foundTexture)
			{
				//we have the texture and it's in the manifest - just reload
				auto handle = texturePacksCache.mAssetCache->ReloadAsset(textureAsset);

				R2_CHECK(!r2::asset::IsInvalidAssetHandle(handle), "We couldn't reload the asset");
			}
			else if (foundInFlatTexturePack && !foundTexture)
			{
				//we have the texture in the manifest, not loaded - added new texture
				tex::Texture newTexture;

				newTexture.textureAssetHandle = texturePacksCache.mAssetCache->LoadAsset(textureAsset);

				R2_CHECK(!r2::asset::IsInvalidAssetHandle(newTexture.textureAssetHandle), "We couldn't load the asset");

				r2::sarr::Push(*loadedTexturePack.textures, newTexture);
			}
			else if (!foundInFlatTexturePack && foundTexture)
			{
				//we have the texture but it's not in the manifest - removed the texture
				texturePacksCache.mAssetCache->FreeAsset(foundTexture->textureAssetHandle);

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
			for (flatbuffers::uoffset_t i = 0; i < foundPack->textures()->size(); ++i)
			{
				if(foundPack->textures()->Get(i)->assetName()->assetName() == textureName)
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
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
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
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
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
		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
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

	bool LoadMaterialTextures(TexturePacksCache& texturePacksCache, const flat::Material* material)
	{
		if (material == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		auto textureParams = material->shaderParams()->textureParams();
		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, textureParams->size());
		AddTexturePacksToTexturePackSet(material, *texturePacks);
		draw::texche::LoadTexturePacks(texturePacksCache, *texturePacks);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	bool LoadMaterialTextures(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialPack)
	{
		if (materialPack == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		u32 numTexturePacks = materialPack->pack()->size() * draw::tex::NUM_TEXTURE_TYPES;

		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, numTexturePacks);

		for (flatbuffers::uoffset_t i = 0; i < materialPack->pack()->size(); ++i)
		{
			const flat::Material* materialParams = materialPack->pack()->Get(i);
			AddTexturePacksToTexturePackSet(materialParams, *texturePacks);
		}

		draw::texche::LoadTexturePacks(texturePacksCache, *texturePacks);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	void GetTexturesForMaterialnternal(TexturePacksCache& texturePacksCache, r2::SArray<u64>* texturePacks, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{

		for (u32 i = 0; i < r2::sarr::Size(*texturePacks); ++i)
		{
			const u64 texturePackName = r2::sarr::At(*texturePacks, i);

			bool isCubemap = r2::draw::texche::IsTexturePackACubemap(texturePacksCache, texturePackName);

			if (!isCubemap && textures != nullptr)
			{
				const r2::SArray<r2::draw::tex::Texture>* texturePackTextures = r2::draw::texche::GetTexturesForTexturePack(texturePacksCache, texturePackName);
				R2_CHECK(texturePackTextures != nullptr, "");
				r2::sarr::Append(*textures, *texturePackTextures);
			}
			else if (isCubemap && cubemaps != nullptr)
			{
				const r2::draw::tex::CubemapTexture* cubemap = r2::draw::texche::GetCubemapTextureForTexturePack(texturePacksCache, texturePackName);

				R2_CHECK(cubemap != nullptr, "");

				r2::sarr::Push(*cubemaps, *cubemap);
			}
			else
			{
				R2_CHECK(false, "hmm");
			}
		}
	}

	const void* GetTextureData(TexturePacksCache& texturePacksCache, const r2::draw::tex::Texture& texture)
	{
		r2::asset::AssetCacheRecord assetCacheRecord = texturePacksCache.mAssetCache->GetAssetBuffer(texture.textureAssetHandle);

		R2_CHECK(assetCacheRecord.GetAssetBuffer()->IsLoaded(), "Not loaded?");

		const void* textureData = reinterpret_cast<const void*>(assetCacheRecord.GetAssetBuffer()->Data());

		texturePacksCache.mAssetCache->ReturnAssetBuffer(assetCacheRecord);

		return textureData;
	}

	u64 GetTextureDataSize(TexturePacksCache& texturePacksCache, const r2::draw::tex::Texture& texture)
	{
		r2::asset::AssetCacheRecord assetCacheRecord = texturePacksCache.mAssetCache->GetAssetBuffer(texture.textureAssetHandle);

		R2_CHECK(assetCacheRecord.GetAssetBuffer()->IsLoaded(), "Not loaded?");

		auto textureSize = assetCacheRecord.GetAssetBuffer()->Size();

		texturePacksCache.mAssetCache->ReturnAssetBuffer(assetCacheRecord);

		return textureSize;
	}

	bool GetTexturesForFlatMaterial(TexturePacksCache& texturePacksCache, const flat::Material* material, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{
		//for this method specifically, we only want to get the specific textures of this material, no others from the packs
		if (material == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		auto textureParams = material->shaderParams()->textureParams();

		for (flatbuffers::uoffset_t i = 0; i < textureParams->size(); ++i)
		{
			if (textureParams->Get(i)->value()->assetName() == EMPTY_TEXTURE_PACK_NAME)
			{
				continue;
			}

			if (!r2::draw::texche::IsTexturePackACubemap(texturePacksCache, textureParams->Get(i)->texturePack()->assetName()))
			{
				const r2::draw::tex::Texture* texture = r2::draw::texche::GetTextureFromTexturePack(texturePacksCache, textureParams->Get(i)->texturePack()->assetName(), textureParams->Get(i)->value()->assetName());

				if (texture)
				{
					r2::sarr::Push(*textures, *texture);
				}
			}
			else
			{
				const r2::draw::tex::CubemapTexture* cubemap = r2::draw::texche::GetCubemapTextureForTexturePack(texturePacksCache, textureParams->Get(i)->texturePack()->assetName());

				R2_CHECK(cubemap != nullptr, "Should never be nullptr");

				r2::sarr::Push(*cubemaps, *cubemap);
			}
		}

		return true;
	}

	bool GetTexturesForMaterialPack(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialPack, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps)
	{
		if (materialPack == nullptr)
		{
			R2_CHECK(false, "Should never be nullptr");
			return false;
		}

		r2::SArray<u64>* texturePacks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, materialPack->pack()->size() * draw::tex::NUM_TEXTURE_TYPES);

		for (flatbuffers::uoffset_t i = 0; i < materialPack->pack()->size(); ++i)
		{
			const flat::Material* material = materialPack->pack()->Get(i);
			AddTexturePacksToTexturePackSet(material, *texturePacks);
		}

		GetTexturesForMaterialnternal(texturePacksCache, texturePacks, textures, cubemaps);

		FREE(texturePacks, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	const r2::draw::tex::Texture* GetAlbedoTextureForMaterialName(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialPack, u64 materialName)
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

		const r2::SArray<r2::draw::tex::Texture>* textures = draw::texche::GetTexturesForTexturePack(texturePacksCache, materialPack->pack()->Get(materialParamsPackIndex)->shaderParams()->textureParams()->Get(materialTexParamsIndex)->texturePack()->assetName());

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

	const r2::draw::tex::CubemapTexture* GetCubemapTextureForMaterialName(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialPack, u64 materialName)
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

		return r2::draw::texche::GetCubemapTextureForTexturePack(texturePacksCache, materialPack->pack()->Get(materialParamsPackIndex)->shaderParams()->textureParams()->Get(materialTexParamsIndex)->texturePack()->assetName());
	}

	

	

}