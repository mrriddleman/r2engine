#ifndef __TEXTURE_PACKS_CACHE_H__
#define __TEXTURE_PACKS_CACHE_H__

//#include "r2/Core/Memory/Memory.h"
//#include "r2/Core/Memory/Allocators/StackAllocator.h"
//#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
//#include "r2/Core/Assets/AssetCache.h"
//#include "r2/Render/Model/Textures/Texture.h"
//#include "r2/Render/Model/Textures/TexturePack.h"
//
//namespace flat
//{
//	struct TexturePacksManifest;
//	struct TexturePackMetaData;
//}
//
//namespace r2::draw
//{
//	struct LoadedTexturePack
//	{
//		u64 packName = 0;
//		const flat::TexturePackMetaData* metaData = nullptr;
//		r2::SArray<r2::draw::tex::Texture>* textures = nullptr;
//		tex::CubemapTexture cubemap;
//	};
//
//	struct TexturePackManifestEntry
//	{
//		const flat::TexturePacksManifest* flatTexturePacksManifest = nullptr;
//		r2::asset::AssetCacheRecord assetCacheRecord;
//	};
//
//	struct TexturePacksCache
//	{
//		u32 mName;
//		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
//		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
//		r2::mem::utils::MemBoundary mAssetBoundary;
//
//		r2::mem::StackArena* mArena;
//		
//		r2::SArray<TexturePackManifestEntry>* mTexturePackManifests;
//		r2::SHashMap<LoadedTexturePack>* mLoadedTexturePacks;
//		
//		r2::mem::FreeListArena* mTexturePackArena;
//		
//		r2::asset::AssetCache* mAssetCache;
//
//		static u64 MemorySize(u32 numTextures, u32 numManifests, u32 numTexturePacks, u32 cacheSize);
//	};
//}
//
//namespace r2::draw::texche
//{
//	struct TexturePacksManifestHandle
//	{
//		u32 cacheName;
//		s32 handle;
//	};
//
//	TexturePacksManifestHandle InvalidTexturePacksManifestHandle = { 0, -1 };
//
//	TexturePacksCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numTextures, u32 numManifests, u32 numTexturePacks, u32 textureCacheSize, const char* areaName);
//	void Shutdown(TexturePacksCache* cache);
//
//	bool GetTexturePacksCacheSizes(const char* texturePacksManifestPath, u32& numTextures, u32& numTexturePacks, u32& cacheSize);
//
//	TexturePacksManifestHandle AddTexturePacksManifestFile(TexturePacksCache& texturePacksCache, const char* texturePacksManifestFilePath);
//	bool RemoveTexturePacksManifestFile(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
//
//	bool LoadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
//	bool LoadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
//
//	bool UnloadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle);
//	bool UnloadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
//
//	const r2::SArray<tex::Texture>* GetTexturesForTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
//	const tex::CubemapTexture* GetCubemapTextureForTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName);
//}

#endif // __TEXTURE_PACKS_CACHE_H__
