#include "r2pch.h"

//#include "r2/Render/Model/Textures/TexturePacksCache.h"
//#include "r2/Core/Memory/InternalEngineMemory.h"
//#include "r2/Core/File/FileSystem.h"
//#include "r2/Core/File/PathUtils.h"
//#include "r2/Core/Assets/AssetLib.h"
//#include "r2/Core/Assets/AssetBuffer.h"
//#include "r2/Core/Assets/AssetFiles/AssetFile.h"
//#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
//#include "r2/Render/Model/Textures/TexturePackMetaData_generated.h"
//#include "r2/Utils/Hash.h"
//#include <algorithm>
//
//namespace r2::draw
//{
//	u64 TexturePacksCache::MemorySize(u32 numTextures, u32 numManifests, u32 numTexturePacks, u32 cacheSize)
//	{
//		u64 memorySize = 0;
//
//		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
//		u32 freeListHeaderSize = r2::mem::FreeListAllocator::HeaderSize();
//
//		u32 boundsChecking = 0;
//#ifdef R2_DEBUG
//		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
//#endif
//
//		const u32 ALIGNMENT = 16;
//
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TexturePacksCache), ALIGNMENT, stackHeaderSize, boundsChecking);
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, stackHeaderSize, boundsChecking);
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::FreeListArena), ALIGNMENT, stackHeaderSize, boundsChecking);
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<TexturePackManifestEntry>::MemorySize(numManifests), ALIGNMENT, stackHeaderSize, boundsChecking);
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<LoadedTexturePack>::MemorySize(numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier()), ALIGNMENT, stackHeaderSize, boundsChecking);
//		memorySize += r2::asset::AssetCache::TotalMemoryNeeded(stackHeaderSize, boundsChecking, static_cast<u64>(numTextures) + static_cast<u64>(numManifests), cacheSize, ALIGNMENT, std::max(1024u, numTextures+ numManifests), std::max(1024u, numTextures+ numManifests));
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::SArray<tex::Texture>), ALIGNMENT, freeListHeaderSize, boundsChecking) * numTexturePacks;
//		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(tex::Texture), ALIGNMENT, freeListHeaderSize, boundsChecking) * numTextures;
//
//		return memorySize;
//	}
//}
//
//namespace r2::draw::texche 
//{
//	const u32 ALIGNMENT = 16;
//	constexpr u32 NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME = 2;
//	void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList);
//	void RemoveAllTexturePathsInTexturePackFromFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList);
//
//	TexturePacksCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numTextures, u32 numManifests, u32 numTexturePacks, u32 textureCacheSize, const char* areaName)
//	{
//		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
//
//		R2_CHECK(memoryArea != nullptr, "Memory area is null?");
//
//		u64 subAreaSize = TexturePacksCache::MemorySize(numTextures, numManifests, numTexturePacks, textureCacheSize);
//		if (memoryArea->UnAllocatedSpace() < subAreaSize)
//		{
//			R2_CHECK(false, "We don't have enough space to allocate the model system! We have: %llu bytes left but trying to allocate: %llu bytes, difference: %llu",
//				memoryArea->UnAllocatedSpace(), subAreaSize, subAreaSize - memoryArea->UnAllocatedSpace());
//			return nullptr;
//		}
//
//		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
//
//		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, areaName)) == r2::mem::MemoryArea::SubArea::Invalid)
//		{
//			R2_CHECK(false, "We couldn't create a sub area for %s", areaName);
//			return nullptr;
//		}
//
//		r2::mem::StackArena* texturePacksCacheArena = EMPLACE_STACK_ARENA(*memoryArea->GetSubArea(subAreaHandle));
//
//		R2_CHECK(texturePacksCacheArena != nullptr, "We couldn't emplace the stack arena - no way to recover!");
//
//		TexturePacksCache* newTexturePacksCache = ALLOC(TexturePacksCache, *texturePacksCacheArena);
//
//		R2_CHECK(newTexturePacksCache != nullptr, "We couldn't create the TexturePacksCache object");
//
//		newTexturePacksCache->mMemoryAreaHandle = memoryAreaHandle;
//		newTexturePacksCache->mSubAreaHandle = subAreaHandle;
//		newTexturePacksCache->mArena = texturePacksCacheArena;
//
//		newTexturePacksCache->mTexturePackManifests = MAKE_SARRAY(*texturePacksCacheArena, TexturePackManifestEntry, numManifests);
//
//		R2_CHECK(newTexturePacksCache->mTexturePackManifests != nullptr, "We couldn't create the texture pack manifests array");
//		
//		TexturePackManifestEntry emptyEntry = {};
//
//		r2::sarr::Fill(*newTexturePacksCache->mTexturePackManifests, emptyEntry);
//
//
//		newTexturePacksCache->mLoadedTexturePacks = MAKE_SHASHMAP(*texturePacksCacheArena, LoadedTexturePack, numTexturePacks * r2::SHashMap<u32>::LoadFactorMultiplier());
//
//		R2_CHECK(newTexturePacksCache->mLoadedTexturePacks != nullptr, "We couldn't create the mLoadedTexturePacks hashmap");
//
//		//first we need to calculate how big this arena is
//		u32 freelistArenaSize = sizeof(r2::SArray<tex::Texture>) * numTexturePacks + sizeof(tex::Texture) * numTextures;
//
//		newTexturePacksCache->mTexturePackArena = MAKE_FREELIST_ARENA(*texturePacksCacheArena, freelistArenaSize, r2::mem::FIND_BEST);
//
//		R2_CHECK(newTexturePacksCache->mTexturePackArena != nullptr, "We couldn't create the the texture pack arena");
//
//		newTexturePacksCache->mAssetBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(*texturePacksCacheArena, textureCacheSize, ALIGNMENT, areaName);
//
//		r2::asset::FileList fileList = r2::asset::lib::MakeFileList(static_cast<u64>(numTextures) + static_cast<u64>(numManifests));
//
//		newTexturePacksCache->mAssetCache = r2::asset::lib::CreateAssetCache(newTexturePacksCache->mAssetBoundary, fileList);
//
//		newTexturePacksCache->mName = r2::utils::HashBytes32(areaName, strlen(areaName));
//
//		return newTexturePacksCache;
//	}
//
//	void Shutdown(TexturePacksCache* texturePacksCache)
//	{
//		if (!texturePacksCache)
//		{
//			return;
//		}
//
//		r2::mem::StackArena* arena = texturePacksCache->mArena;
//
//		const u32 numTexturePackManifests = r2::sarr::Size(*texturePacksCache->mTexturePackManifests);
//
//		for (u32 i = 0; i < numTexturePackManifests; ++i)
//		{
//			const auto& entry = r2::sarr::At(*texturePacksCache->mTexturePackManifests, i);
//
//			if (entry.flatTexturePacksManifest != nullptr && entry.assetCacheRecord.buffer != nullptr)
//			{
//				UnloadAllTexturesFromTexturePacksManifestFromDisk(*texturePacksCache, { texturePacksCache->mName, static_cast<s32>(i) });
//
//				texturePacksCache->mAssetCache->ReturnAssetBuffer(entry.assetCacheRecord);
//			}
//		}
//
//		r2::sarr::Clear(*texturePacksCache->mTexturePackManifests);
//		r2::shashmap::Clear(*texturePacksCache->mLoadedTexturePacks);
//
//		mem::utils::MemBoundary assetBoundary = texturePacksCache->mAssetBoundary;
//
//		r2::asset::lib::DestroyCache(texturePacksCache->mAssetCache);
//
//		FREE(assetBoundary.location, *arena);
//
//		FREE(texturePacksCache->mTexturePackArena, *arena);
//
//		FREE(texturePacksCache->mLoadedTexturePacks, *arena);
//
//		FREE(texturePacksCache->mTexturePackManifests, *arena);
//
//		FREE(texturePacksCache, *arena);
//
//		FREE_EMPLACED_ARENA(arena);
//	}
//
//	bool GetTexturePacksCacheSizes(const char* texturePacksManifestPath, u32& numTextures, u32& numTexturePacks, u32& cacheSize)
//	{
//		if (!texturePacksManifestPath)
//		{
//			return false;
//		}
//
//		R2_CHECK(strlen(texturePacksManifestPath) > 0, "We shouldn't have an improper path here");
//
//		u64 fileSize = 0;
//		void* fileData = r2::fs::ReadFile<r2::mem::StackArena>(*MEM_ENG_SCRATCH_PTR, texturePacksManifestPath, fileSize);
//
//		if (!fileData)
//		{
//			R2_CHECK(false, "Unable to read the data from: %s\n", texturePacksManifestPath);
//			return false;
//		}
//
//		const flat::TexturePacksManifest* texturePacksManifest = flat::GetTexturePacksManifest(fileData);
//
//		R2_CHECK(texturePacksManifest != nullptr, "Somehow we couldn't get the proper flat::TexturePacksManifest data!");
//
//		cacheSize = static_cast<u32>(texturePacksManifest->totalTextureSize()) + static_cast<u32>(fileSize); //+ the file size if we want to keep that around
//		numTextures = static_cast<u32>(texturePacksManifest->totalNumberOfTextures());
//		numTexturePacks = static_cast<u32>(texturePacksManifest->texturePacks()->size());
//		
//		FREE(fileData, *MEM_ENG_SCRATCH_PTR);
//
//		return true;
//	}
//
//	TexturePacksManifestHandle AddTexturePacksManifestFile(TexturePacksCache& texturePacksCache, const char* texturePacksManifestFilePath)
//	{
//		//add the texturePacksManifestFilePath file 
//		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the cache yet");
//			return InvalidTexturePacksManifestHandle;
//		}
//		
//		if (texturePacksManifestFilePath == nullptr || strlen(texturePacksManifestFilePath) == 0)
//		{
//			R2_CHECK(false, "Passed in an invalid texture pack manifest file path!");
//			return InvalidTexturePacksManifestHandle;
//		}
//
//		r2::asset::FileList fileList = texturePacksCache.mAssetCache->GetFileList();
//
//		r2::asset::Asset manifestAsset = r2::asset::Asset::MakeAssetFromFilePath(texturePacksManifestFilePath, r2::asset::TEXTURE_PACK_MANIFEST);//r2::asset::Asset(texturePacksManifestName, r2::asset::TEXTURE_PACK_MANIFEST);
//
//		const auto numTexturePackManifests = r2::sarr::Capacity(*texturePacksCache.mTexturePackManifests);
//		bool found = false;
//		s32 index = -1;
//		for (u32 i = 0; i < numTexturePackManifests; ++i)
//		{
//			const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, i);
//			if (entry.flatTexturePacksManifest == nullptr)
//			{
//				continue;
//			}
//
//			if (entry.assetCacheRecord.handle.handle == manifestAsset.HashID())
//			{
//				found = true;
//				index = i;
//				break;
//			}
//		}
//
//		if (found)
//		{
//			return { texturePacksCache.mName, index };
//		}
//
//		r2::asset::RawAssetFile* texturePacksManifestFile = r2::asset::lib::MakeRawAssetFile(texturePacksManifestFilePath, 0);
//
//		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)texturePacksManifestFile);
//
//		TexturePackManifestEntry newEntry;
//
//		r2::asset::AssetHandle assetHandle = texturePacksCache.mAssetCache->LoadAsset(manifestAsset);
//
//		if (r2::asset::IsInvalidAssetHandle(assetHandle))
//		{
//			R2_CHECK(false, "Failed to load the texture packs manifest file name: %s", texturePacksManifestFilePath);
//			return InvalidTexturePacksManifestHandle;
//		}
//
//		newEntry.assetCacheRecord = texturePacksCache.mAssetCache->GetAssetBuffer(assetHandle);
//
//		R2_CHECK(newEntry.assetCacheRecord.buffer != nullptr, "We should have a proper buffer here");
//
//		newEntry.flatTexturePacksManifest = flat::GetTexturePacksManifest(newEntry.assetCacheRecord.buffer->Data());
//
//		R2_CHECK(newEntry.flatTexturePacksManifest != nullptr, "We should have the flatbuffer data");
//
//		TexturePacksManifestHandle newHandle = { texturePacksCache.mName, -1 };
//
//		for (u32 i = 0; i < numTexturePackManifests; ++i)
//		{
//			const TexturePackManifestEntry& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, i);
//
//			if (entry.flatTexturePacksManifest == nullptr)
//			{
//				r2::sarr::At(*texturePacksCache.mTexturePackManifests, i) = newEntry;
//				newHandle.handle = static_cast<s32>(i);
//				break;
//			}
//		}
//
//		const flat::TexturePacksManifest* manifest = newEntry.flatTexturePacksManifest;
//
//		for (flatbuffers::uoffset_t i = 0; i < manifest->texturePacks()->size(); ++i)
//		{
//			const flat::TexturePack* texturePack = manifest->texturePacks()->Get(i);
//
//			AddAllTexturePathsInTexturePackToFileList(texturePack, fileList);
//		}
//
//		return newHandle;
//	}
//
//	bool RemoveTexturePacksManifestFile(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
//	{
//		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mTexturePackManifests == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the cache yet");
//			return false;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return false;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Capacity(*texturePacksCache.mTexturePackManifests));
//			return false;
//		}
//
//		auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);
//
//		r2::asset::Asset manifestAsset = r2::asset::Asset(entry.assetCacheRecord.handle.handle, r2::asset::TEXTURE_PACK_MANIFEST);
//
//		r2::asset::FileList fileList = texturePacksCache.mAssetCache->GetFileList();
//
//		const auto numFiles = r2::sarr::Size(*fileList);
//		bool found = false;
//		s32 foundIndex = -1;
//		for (u32 i = 0; i < numFiles && !found; ++i)
//		{
//			const auto& file = r2::sarr::At(*fileList, i);
//
//			const auto numAssets = file->NumAssets();
//
//			for (u32 j =0; j < numAssets && !found; ++j)
//			{
//				if (file->GetAssetHandle(j) == manifestAsset.HashID())
//				{
//					found = true;
//					foundIndex = i;
//				}
//			}
//		}
//		
//		if (found)
//		{
//			//remove from the list
//			r2::sarr::RemoveAndSwapWithLastElement(*fileList, foundIndex);
//		}
//
//		for (u32 i = 0; i < entry.flatTexturePacksManifest->texturePacks()->size(); ++i)
//		{
//			RemoveAllTexturePathsInTexturePackFromFileList(entry.flatTexturePacksManifest->texturePacks()->Get(i), fileList);
//		}
//
//		texturePacksCache.mAssetCache->ReturnAssetBuffer(entry.assetCacheRecord);
//		entry.assetCacheRecord.buffer = nullptr;
//		entry.assetCacheRecord.handle = {};
//		entry.flatTexturePacksManifest = nullptr;
//
//		return true;
//	}
//
//	void LoadTextures(
//		LoadedTexturePack& loadedTexturePack,
//		r2::asset::AssetCache* assetCache,
//		const flat::TexturePack* texturePack,
//		const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* paths,
//		r2::draw::tex::TextureType type)
//	{
//		char assetName[r2::fs::FILE_PATH_LENGTH];
//
//		for (u32 i = 0; i < paths->size(); ++i)
//		{
//			const flatbuffers::String* texturePath = paths->Get(i);
//
//			r2::fs::utils::CopyFileNameWithParentDirectories(texturePath->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
//
//			r2::asset::Asset textureAsset(assetName, r2::asset::TEXTURE);
//
//			tex::Texture newTexture;
//			newTexture.type = type;
//
//			newTexture.textureAssetHandle = assetCache->LoadAsset(textureAsset);
//
//			R2_CHECK(!r2::asset::IsInvalidAssetHandle(newTexture.textureAssetHandle), "We couldn't load the texture: %s!", texturePath->c_str());
//
//			r2::sarr::Push(*loadedTexturePack.textures, newTexture);
//		}
//	}
//
//	bool LoadAllTexturesFromTexturePackFromDisk(TexturePacksCache& texturePacksCache, const flat::TexturePack* texturePack)
//	{
//		if (texturePack == nullptr)
//		{
//			return false;
//		}
//
//		const auto numTexturesInThePack = texturePack->totalNumberOfTextures();
//
//		LoadedTexturePack loadedTexturePack;
//		
//		loadedTexturePack.packName = texturePack->packName();
//		loadedTexturePack.metaData = texturePack->metaData();
//
//		if (loadedTexturePack.metaData->type() == flat::TextureType_TEXTURE)
//		{
//			loadedTexturePack.textures = MAKE_SARRAY(*texturePacksCache.mTexturePackArena, tex::Texture, numTexturesInThePack);
//			
//			//now load all of the textures of this pack
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->albedo(), tex::TextureType::Diffuse);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->anisotropy(), tex::TextureType::Anisotropy);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->clearCoat(), tex::TextureType::ClearCoat);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->clearCoatNormal(), tex::TextureType::ClearCoatNormal);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->clearCoatRoughness(), tex::TextureType::ClearCoatRoughness);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->detail(), tex::TextureType::Detail);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->emissive(), tex::TextureType::Emissive);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->height(), tex::TextureType::Height);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->metallic(), tex::TextureType::Metallic);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->normal(), tex::TextureType::Normal);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->occlusion(), tex::TextureType::Occlusion);
//			LoadTextures(loadedTexturePack, texturePacksCache.mAssetCache, texturePack, texturePack->roughness(), tex::TextureType::Roughness);
//		}
//		else if (loadedTexturePack.metaData->type() == flat::TextureType_CUBEMAP)
//		{
//			char assetName[r2::fs::FILE_PATH_LENGTH];
//			loadedTexturePack.cubemap.numMipLevels = loadedTexturePack.metaData->mipLevels()->size();
//
//			for (u32 m = 0; m < loadedTexturePack.cubemap.numMipLevels; ++m)
//			{
//				const auto mipLevel = texturePack->metaData()->mipLevels()->Get(m);
//
//				const auto numSides = mipLevel->sides()->size();
//
//				loadedTexturePack.cubemap.mips[m].mipLevel = mipLevel->level();
//
//				for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
//				{
//					const auto side = mipLevel->sides()->Get(i);
//
//					r2::fs::utils::CopyFileNameWithParentDirectories(side->textureName()->c_str(), assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
//					r2::asset::Asset textureAsset(assetName, r2::asset::CUBEMAP_TEXTURE);
//
//					r2::draw::tex::Texture texture;
//					texture.type = tex::TextureType::Diffuse;
//
//					loadedTexturePack.cubemap.mips[m].sides[i] = texture;
//				}
//			}
//		}
//
//		r2::shashmap::Set(*texturePacksCache.mLoadedTexturePacks, loadedTexturePack.packName, loadedTexturePack);
//
//		return true;
//	}
//
//	bool LoadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
//	{
//		if (texturePacksCache.mLoadedTexturePacks == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
//			return false;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return false;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
//			return false;
//		}
//
//		const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);
//
//		const flat::TexturePacksManifest* texturePacksManifest = entry.flatTexturePacksManifest;
//
//		
//		for (flatbuffers::uoffset_t i = 0; i < texturePacksManifest->texturePacks()->size(); ++i)
//		{
//			//@NOTE(Serge): we want to use this function since it checks to see if we have it loaded already
//			LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->packName());
//		}
//
//		return true;
//	}
//
//	bool LoadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
//	{
//		if (texturePacksCache.mLoadedTexturePacks == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
//			return false;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return false;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
//			return false;
//		}
//
//		//first check to see if we have it already
//		LoadedTexturePack defaultLoadedTexturePack;
//
//		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
//
//		if (resultTexturePack.packName != defaultLoadedTexturePack.packName)
//		{
//			return true;
//		}
//
//		const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);
//
//		const flat::TexturePacksManifest* texturePacksManifest = entry.flatTexturePacksManifest;
//
//		//find the texture pack
//		const flat::TexturePack* texturePack = nullptr;
//		for (flatbuffers::uoffset_t i = 0; i < texturePacksManifest->texturePacks()->size(); ++i)
//		{
//			if (texturePacksManifest->texturePacks()->Get(i)->packName() == texturePackName)
//			{
//				texturePack = texturePacksManifest->texturePacks()->Get(i);
//				break;
//			}
//		}
//
//		if (texturePack == nullptr)
//		{
//			R2_CHECK(false, "We couldn't find the texturePack for packName: %llu", texturePackName);
//			return false;
//		}
//
//		return LoadAllTexturesFromTexturePackFromDisk(texturePacksCache, texturePack);
//	}
//
//	bool UnloadAllTexturesFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle)
//	{
//		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
//			return false;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return false;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
//			return false;
//		}
//
//		const auto& entry = r2::sarr::At(*texturePacksCache.mTexturePackManifests, handle.handle);
//
//		const flat::TexturePacksManifest* texturePacksManifest = entry.flatTexturePacksManifest;
//
//		for (flatbuffers::uoffset_t i = 0; i < texturePacksManifest->texturePacks()->size(); ++i)
//		{
//			//@NOTE(Serge): we want to use this function since it checks to see if we have it loaded already
//			UnloadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePacksManifest->texturePacks()->Get(i)->packName());
//		}
//
//		return true;
//	}
//
//	bool UnloadTexturePackFromTexturePacksManifestFromDisk(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
//	{
//		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
//			return false;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return false;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
//			return false;
//		}
//
//		//first check to see if we even loaded the pack
//		{
//			LoadedTexturePack defaultLoadedTexturePack;
//			
//			LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
//
//			if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
//			{
//				//we don't have it loaded so nothing to do!
//				return true;
//			}
//
//			//we did find the loaded files - now unload them
//			if (resultTexturePack.metaData->type() == flat::TextureType_TEXTURE)
//			{
//				const auto numTextures = r2::sarr::Size(*resultTexturePack.textures);
//
//				for (u32 i = 0; i < numTextures; ++i)
//				{
//					const tex::Texture& texture = r2::sarr::At(*resultTexturePack.textures, i);
//
//					texturePacksCache.mAssetCache->FreeAsset(texture.textureAssetHandle);
//				}
//
//				FREE(resultTexturePack.textures, *texturePacksCache.mTexturePackArena);
//			}
//			else
//			{
//				for (u32 m = 0; m < resultTexturePack.cubemap.numMipLevels; ++m)
//				{
//					const auto mipLevel = resultTexturePack.metaData->mipLevels()->Get(m);
//
//					const auto numSides = mipLevel->sides()->size();
//
//					for (flatbuffers::uoffset_t i = 0; i < numSides; ++i)
//					{
//						texturePacksCache.mAssetCache->FreeAsset(resultTexturePack.cubemap.mips[m].sides[i].textureAssetHandle);	
//					}
//				}
//			}
//		}
//		
//		r2::shashmap::Remove(*texturePacksCache.mLoadedTexturePacks, texturePackName);
//
//		return true;
//	}
//
//	void AddAllTexturesFromTextureType(const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* texturePaths, r2::asset::FileList fileList)
//	{
//		for (u32 i = 0; i < texturePaths->size(); ++i)
//		{
//			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(texturePaths->Get(i)->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
//			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
//		}
//	}
//
//	void AddAllTexturePathsInTexturePackToFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList)
//	{
//		AddAllTexturesFromTextureType(texturePack->albedo(), fileList);
//		AddAllTexturesFromTextureType(texturePack->anisotropy(), fileList);
//		AddAllTexturesFromTextureType(texturePack->clearCoat(), fileList);
//		AddAllTexturesFromTextureType(texturePack->clearCoatNormal(), fileList);
//		AddAllTexturesFromTextureType(texturePack->clearCoatRoughness(), fileList);
//		AddAllTexturesFromTextureType(texturePack->detail(), fileList);
//		AddAllTexturesFromTextureType(texturePack->emissive(), fileList);
//		AddAllTexturesFromTextureType(texturePack->height(), fileList);
//		AddAllTexturesFromTextureType(texturePack->metallic(), fileList);
//		AddAllTexturesFromTextureType(texturePack->normal(), fileList);
//		AddAllTexturesFromTextureType(texturePack->occlusion(), fileList);
//		AddAllTexturesFromTextureType(texturePack->roughness(), fileList);
//
//		if (texturePack->metaData() && texturePack->metaData()->mipLevels())
//		{
//			for (flatbuffers::uoffset_t i = 0; i < texturePack->metaData()->mipLevels()->size(); ++i)
//			{
//				const flat::MipLevel* mipLevel = texturePack->metaData()->mipLevels()->Get(i);
//
//				for (flatbuffers::uoffset_t side = 0; side < mipLevel->sides()->size(); ++side)
//				{
//					r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(mipLevel->sides()->Get(side)->textureName()->c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
//					r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
//				}
//			}
//		}
//	}
//
//	void RemoveFilePathFromFileList(const char* filePath, r2::asset::FileList fileList)
//	{
//		char assetName[r2::fs::FILE_PATH_LENGTH];
//
//		r2::fs::utils::CopyFileNameWithParentDirectories(filePath, assetName, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_ASSET_NAME);
//		r2::asset::Asset theAsset = r2::asset::Asset(assetName, r2::asset::TEXTURE);
//
//		const auto fileSize = r2::sarr::Size(*fileList);
//
//		s32 foundIndex = -1;
//		bool found = false;
//
//		for (u32 j = 0; j < fileSize && !found; ++j)
//		{
//			r2::asset::AssetFile* assetFile = r2::sarr::At(*fileList, j);
//
//			const u32 numAssets = assetFile->NumAssets();
//
//			for (u32 k = 0; k < numAssets && !found; ++k)
//			{
//				if (assetFile->GetAssetHandle(k) == theAsset.HashID())
//				{
//					foundIndex = j;
//					found = true;
//				}
//			}
//		}
//
//		if (!found)
//		{
//			return;
//		}
//
//		r2::sarr::RemoveElementAtIndexShiftLeft(*fileList, foundIndex);
//	}
//
//	void RemoveAllTexturesFromFileListForTextureType(const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>* texturePaths, r2::asset::FileList fileList)
//	{
//		for (u32 i = 0; i < texturePaths->size(); ++i)
//		{
//			RemoveFilePathFromFileList(texturePaths->Get(i)->c_str(), fileList);
//		}
//	}
//
//	void RemoveAllTexturePathsInTexturePackFromFileList(const flat::TexturePack* texturePack, r2::asset::FileList fileList)
//	{
//		RemoveAllTexturesFromFileListForTextureType(texturePack->albedo(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->anisotropy(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->clearCoat(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->clearCoatNormal(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->clearCoatRoughness(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->detail(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->emissive(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->height(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->metallic(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->normal(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->occlusion(), fileList);
//		RemoveAllTexturesFromFileListForTextureType(texturePack->roughness(), fileList);
//
//		if (texturePack->metaData() && texturePack->metaData()->mipLevels())
//		{
//			for (flatbuffers::uoffset_t i = 0; i < texturePack->metaData()->mipLevels()->size(); ++i)
//			{
//				const flat::MipLevel* mipLevel = texturePack->metaData()->mipLevels()->Get(i);
//
//				for (flatbuffers::uoffset_t side = 0; side < mipLevel->sides()->size(); ++side)
//				{
//					RemoveFilePathFromFileList(mipLevel->sides()->Get(side)->textureName()->c_str(), fileList);
//				}
//			}
//		}
//	}
//
//	const r2::SArray<r2::draw::tex::Texture>* GetTexturesForTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
//	{
//		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
//			return nullptr;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return nullptr;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
//			return nullptr;
//		}
//
//		LoadedTexturePack defaultLoadedTexturePack;
//
//		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
//
//		if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
//		{
//			bool loaded = LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePackName);
//
//			if (!loaded)
//			{
//				R2_CHECK(false, "We couldn't seem to load the texture pack");
//				return nullptr;
//			}
//
//			LoadedTexturePack& loadedTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
//
//			//we haven't loaded the texture pack yet
//			return loadedTexturePack.textures;
//		}
//
//		return resultTexturePack.textures;
//	}
//
//	const tex::CubemapTexture* GetCubemapTextureForTexturePack(TexturePacksCache& texturePacksCache, TexturePacksManifestHandle handle, u64 texturePackName)
//	{
//		if (texturePacksCache.mAssetCache == nullptr || texturePacksCache.mLoadedTexturePacks == nullptr)
//		{
//			R2_CHECK(false, "We haven't initialized the TexturePacksCache yet");
//			return nullptr;
//		}
//
//		if (handle.cacheName != texturePacksCache.mName)
//		{
//			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
//			return nullptr;
//		}
//
//		if (handle.handle < 0 || r2::sarr::Size(*texturePacksCache.mTexturePackManifests) <= handle.handle)
//		{
//			R2_CHECK(false, "handle : %i is not valid since it's not in the range 0 - %ull", handle.handle, r2::sarr::Size(*texturePacksCache.mTexturePackManifests));
//			return nullptr;
//		}
//
//		LoadedTexturePack defaultLoadedTexturePack;
//
//		LoadedTexturePack& resultTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
//
//		if (resultTexturePack.packName == defaultLoadedTexturePack.packName)
//		{
//			bool loaded = LoadTexturePackFromTexturePacksManifestFromDisk(texturePacksCache, handle, texturePackName);
//
//			if (!loaded)
//			{
//				R2_CHECK(false, "We couldn't seem to load the texture pack");
//				return nullptr;
//			}
//
//			LoadedTexturePack& loadedTexturePack = r2::shashmap::Get(*texturePacksCache.mLoadedTexturePacks, texturePackName, defaultLoadedTexturePack);
//
//			//we haven't loaded the texture pack yet
//			return &loadedTexturePack.cubemap;
//		}
//
//		return &resultTexturePack.cubemap;
//	}

//}