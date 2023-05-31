#ifndef __TEXTURE_PACKS_CACHE_H__
#define __TEXTURE_PACKS_CACHE_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Render/Model/Textures/TexturePack.h"

namespace r2
{
	class GameAssetManager;
}

namespace flat
{
	struct TexturePacksManifest;
	struct TexturePackMetaData;
}

namespace r2::draw
{
	struct LoadedTexturePack
	{
		u64 packName = 0;
		const flat::TexturePackMetaData* metaData = nullptr;
		r2::SArray<r2::draw::tex::Texture>* textures = nullptr;
		tex::CubemapTexture cubemap;
	};

	struct TexturePackManifestEntry
	{
		const flat::TexturePacksManifest* flatTexturePacksManifest = nullptr;
	};

	struct TexturePacksCache
	{
		r2::mem::utils::MemBoundary mMemoryBoundary;

		r2::mem::StackArena* mArena;

		r2::SArray<TexturePackManifestEntry>* mTexturePackManifests;
		r2::SHashMap<LoadedTexturePack>* mLoadedTexturePacks;
		r2::SHashMap<s32>* mPackNameToTexturePackManifestEntryMap;
		r2::SHashMap<s32>* mManifestNameToTexturePackManifestEntryMap;

		r2::mem::FreeListArena* mTexturePackArena;

		r2::GameAssetManager* mnoptrGameAssetManager;

		static u64 MemorySize(u32 numTextures, u32 numTextureManifests, u32 numTexturePacks);
	};
}

namespace r2::draw::texche
{
	struct TexturePacksManifestHandle
	{
		s32 handle;
	};

	TexturePacksManifestHandle InvalidTexturePacksManifestHandle = { -1 };

	template<class ARENA>
	TexturePacksCache* Create(ARENA& arena, u32 numTextures, u32 numManifests, u32 numTexturePacks, r2::GameAssetManager* gameAssetManager)
	{
		u64 subAreaSize = TexturePacksCache::MemorySize(numTextures, numManifests, numTexturePacks);

		r2::mem::utils::MemBoundary memBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(arena, subAreaSize, 16, "TexturePacksCacheMemBoundary");

		r2::mem::StackArena* texturePacksCacheArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(memBoundary);

		R2_CHECK(texturePacksCacheArena != nullptr, "We couldn't emplace the stack arena - no way to recover!");

		TexturePacksCache* newTexturePacksCache = ALLOC(TexturePacksCache, *texturePacksCacheArena);

		R2_CHECK(newTexturePacksCache != nullptr, "We couldn't create the TexturePacksCache object");

		newTexturePacksCache->mMemoryBoundary = memBoundary;

		newTexturePacksCache->mArena = texturePacksCacheArena;

		newTexturePacksCache->mTexturePackManifests = MAKE_SARRAY(*texturePacksCacheArena, TexturePackManifestEntry, numManifests);

		R2_CHECK(newTexturePacksCache->mTexturePackManifests != nullptr, "We couldn't create the texture pack manifests array");

		TexturePackManifestEntry emptyEntry = {};

		r2::sarr::Fill(*newTexturePacksCache->mTexturePackManifests, emptyEntry);


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

		newTexturePacksCache->mnoptrGameAssetManager = gameAssetManager;

		return newTexturePacksCache;
	}

	template<class ARENA>
	void Shutdown(ARENA& arena, TexturePacksCache* texturePacksCache)
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

			if (entry.flatTexturePacksManifest != nullptr )
			{
				UnloadAllTexturesFromTexturePacksManifestFromDisk(*texturePacksCache, { texturePacksCache->mName, static_cast<s32>(i) });
			}
		}

		r2::sarr::Clear(*texturePacksCache->mTexturePackManifests);
		r2::shashmap::Clear(*texturePacksCache->mLoadedTexturePacks);
		r2::shashmap::Clear(*texturePacksCache->mPackNameToTexturePackManifestEntryMap);
		r2::shashmap::Clear(*texturePacksCache->mManifestNameToTexturePackManifestEntryMap);

		FREE(texturePacksCache->mTexturePackArena, *texturePacksCacheArena);

		FREE(texturePacksCache->mManifestNameToTexturePackManifestEntryMap, *texturePacksCacheArena);

		FREE(texturePacksCache->mPackNameToTexturePackManifestEntryMap, *texturePacksCacheArena);

		FREE(texturePacksCache->mLoadedTexturePacks, *texturePacksCacheArena);

		FREE(texturePacksCache->mTexturePackManifests, *texturePacksCacheArena);

		FREE(texturePacksCache, *texturePacksCacheArena);

		FREE_EMPLACED_ARENA(texturePacksCacheArena);

		FREE(texturePacksCacheBoundary, *arena);
	}

	bool GetTexturePacksCacheSizes(const char* texturePacksManifestPath, u32& numTextures, u32& numTexturePacks, u32& cacheSize);

	TexturePacksManifestHandle AddTexturePacksManifestFile(TexturePacksCache& texturePacksCache, const char* texturePacksManifestFilePath);

	bool LoadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
	bool LoadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, u64 texturePackName);

	bool UnloadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
	bool UnloadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, u64 texturePackName);

	const r2::SArray<tex::Texture>* GetTexturesForTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
	const tex::CubemapTexture* GetCubemapTextureForTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
}

#endif // __TEXTURE_PACKS_CACHE_H__
