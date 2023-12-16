#ifndef __TEXTURE_PACKS_CACHE_H__
#define __TEXTURE_PACKS_CACHE_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Render/Model/Textures/Texture.h"

namespace flat
{
	struct TexturePacksManifest;
	struct TexturePackMetaData;
	struct Material;
	struct MaterialPack;
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

		r2::asset::AssetCache* mAssetCache;
		r2::mem::utils::MemBoundary mAssetCacheBoundary;
		static u64 MemorySize(u32 textureCapacity, u32 numTextures, u32 numTextureManifests, u32 numTexturePacks);
	};
}

namespace r2::draw::texche
{
	struct TexturePacksManifestHandle
	{
		s32 handle;
		static const TexturePacksManifestHandle Invalid;
	};

	
	TexturePacksCache* Create(r2::mem::utils::MemBoundary memBoundary, u32 textureCapacity, u32 numTextures, u32 numManifests, u32 numTexturePacks);
	
	void Shutdown(TexturePacksCache* texturePacksCache);
	

	bool GetTexturePacksCacheSizes(const char* texturePacksManifestPath, u32& numTextures, u32& numTexturePacks, u32& numCubemaps, u32& cacheSize);

	TexturePacksManifestHandle AddTexturePacksManifestFile(TexturePacksCache& texturePacksCache, u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest);
	bool UpdateTexturePacksManifest(TexturePacksCache& texturePacksCache, u64 texturePackHandle, const flat::TexturePacksManifest* texturePacksManifest);

	bool LoadAllTexturePacks(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
	bool LoadTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName);
	bool LoadTexturePacks(TexturePacksCache& texturePacksCache, const r2::SArray<u64>& texturePacks);

	bool UnloadAllTexturePacks(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
	bool UnloadTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName);
	bool UnloadTexture(TexturePacksCache& texturePacksCache, const tex::Texture& texture);

	bool IsTexturePackACubemap(TexturePacksCache& texturePacksCache, u64 texturePackName);

	bool ReloadTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName);
	bool ReloadTextureInTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName, u64 textureName);

	const r2::SArray<tex::Texture>* GetTexturesForTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName);
	const tex::Texture* GetTextureFromTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName, u64 textureName);
	const tex::CubemapTexture* GetCubemapTextureForTexturePack(TexturePacksCache& texturePacksCache, u64 texturePackName);

	bool LoadMaterialTextures(TexturePacksCache& texturePacksCache, const flat::Material* material);
	bool LoadMaterialTextures(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialPack);

	bool GetTexturesForFlatMaterial(TexturePacksCache& texturePacksCache, const flat::Material* material, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);
	bool GetTexturesForMaterialPack(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialParamsPack, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);

	const r2::draw::tex::Texture* GetAlbedoTextureForMaterialName(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialParamsPack, u64 materialName);
	const r2::draw::tex::CubemapTexture* GetCubemapTextureForMaterialName(TexturePacksCache& texturePacksCache, const flat::MaterialPack* materialParamsPack, u64 materialName);
	
	void GetTexturesForMaterialnternal(TexturePacksCache& texturePacksCache, r2::SArray<u64>* texturePacks, r2::SArray<r2::draw::tex::Texture>* textures, r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps);

	const void* GetTextureData(TexturePacksCache& texturePacksCache, const r2::draw::tex::Texture& texture);
	u64 GetTextureDataSize(TexturePacksCache& texturePacksCache, const r2::draw::tex::Texture& texture);
}

#endif // __TEXTURE_PACKS_CACHE_H__
