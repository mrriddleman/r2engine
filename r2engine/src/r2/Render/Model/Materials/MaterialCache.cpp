#include "r2pch.h"

#include "r2/Render/Model/Materials/MaterialCache.h"

#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Materials/MaterialParams_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"

#include "r2/Utils/Hash.h"

namespace r2::draw
{
	u64 MaterialCache::MemorySize(u32 numMaterialParamsManifests, u32 cacheSize)
	{
		u64 memorySize = 0;

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		const u32 ALIGNMENT = 16;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(MaterialCache), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialParamsPackManifestEntry>::MemorySize(numMaterialParamsManifests), ALIGNMENT, stackHeaderSize, boundsChecking);
		memorySize += r2::asset::AssetCache::TotalMemoryNeeded(stackHeaderSize, boundsChecking, numMaterialParamsManifests, cacheSize, ALIGNMENT);

		return memorySize;
	}
}

namespace r2::draw::mtrlche
{
	const u32 ALIGNMENT = 16;

	MaterialCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numMaterialParamsManifests, u32 cacheSize, const char* areaName)
	{
		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);

		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MaterialCache::MemorySize(numMaterialParamsManifests, cacheSize);

		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the model system! We have: %llu bytes left but trying to allocate: %llu bytes, difference: %llu",
				memoryArea->UnAllocatedSpace(), subAreaSize, subAreaSize - memoryArea->UnAllocatedSpace());
			return nullptr;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, areaName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for %s", areaName);
			return nullptr;
		}

		r2::mem::StackArena* materialCacheArena = EMPLACE_STACK_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(materialCacheArena != nullptr, "We couldn't emplace the stack arena");

		MaterialCache* newMaterialCache = ALLOC(MaterialCache, *materialCacheArena);

		R2_CHECK(newMaterialCache != nullptr, "We couldn't allocate the MaterialCache object");

		newMaterialCache->mMemoryAreaHandle = memoryAreaHandle;
		newMaterialCache->mSubAreaHandle = subAreaHandle;
		newMaterialCache->mArena = materialCacheArena;

		newMaterialCache->mMaterialParamsManifests = MAKE_SARRAY(*materialCacheArena, MaterialParamsPackManifestEntry, numMaterialParamsManifests);

		R2_CHECK(newMaterialCache->mMaterialParamsManifests != nullptr, "We couldn't create the material params pack manifest entries!");

		MaterialParamsPackManifestEntry emptyEntry = {};

		r2::sarr::Fill(*newMaterialCache->mMaterialParamsManifests, emptyEntry);

		newMaterialCache->mAssetBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(*materialCacheArena, cacheSize, ALIGNMENT, areaName);

		r2::asset::FileList fileList = r2::asset::lib::MakeFileList(numMaterialParamsManifests);

		newMaterialCache->mAssetCache = r2::asset::lib::CreateAssetCache(newMaterialCache->mAssetBoundary, fileList);

		newMaterialCache->mName = STRING_ID(areaName);

		return newMaterialCache;
	}

	void Shutdown(MaterialCache* materialCache)
	{
		if (!materialCache)
		{
			return;
		}

		const auto manifestCapacity = r2::sarr::Capacity(*materialCache->mMaterialParamsManifests);

		for (u32 i = 0; i < manifestCapacity; ++i)
		{
			const auto& entry = r2::sarr::At(*materialCache->mMaterialParamsManifests, i);

			if (entry.flatMaterialParamsPack != nullptr && entry.assetCacheRecord.buffer != nullptr)
			{
				materialCache->mAssetCache->ReturnAssetBuffer(entry.assetCacheRecord);
			}
		}

		r2::sarr::Clear(*materialCache->mMaterialParamsManifests);

		r2::mem::StackArena* arena = materialCache->mArena;

		mem::utils::MemBoundary assetBoundary = materialCache->mAssetBoundary;

		r2::asset::lib::DestroyCache(materialCache->mAssetCache);

		FREE(assetBoundary.location, *arena);

		FREE(materialCache->mMaterialParamsManifests, *arena);

		FREE(materialCache, *arena);

		FREE_EMPLACED_ARENA(arena);
	}

	bool GetMaterialPacksCacheSizes(const char* materialParamsManifestFilePath, u32& numMaterials, u32& cacheSize)
	{
		if (!materialParamsManifestFilePath)
		{
			R2_CHECK(false, "Passed in nullptr for the materialParamsManifestFilePath");
			return false;
		}

		R2_CHECK(strlen(materialParamsManifestFilePath) > 0, "We shouldn't have an improper path here");
		
		u64 fileSize = 0;
		void* fileData = r2::fs::ReadFile<r2::mem::StackArena>(*MEM_ENG_SCRATCH_PTR, materialParamsManifestFilePath, fileSize);

		if (!fileData)
		{
			R2_CHECK(false, "Unable to read the data from: %s\n", materialParamsManifestFilePath);
			return false;
		}

		const flat::MaterialParamsPack* materialParamsPack = flat::GetMaterialParamsPack(fileData);

		R2_CHECK(materialParamsPack != nullptr, "We couldn't get the proper flat::MaterialParamsPack data");

		cacheSize = static_cast<u32>(fileSize);
		numMaterials = static_cast<u32>(materialParamsPack->pack()->size());

		FREE(fileData, *MEM_ENG_SCRATCH_PTR);

		return true;
	}

	MaterialParamsPackHandle AddMaterialParamsPackFile(MaterialCache& materialCache, const char* materialParamsPackManifestFilePath)
	{
		if (materialCache.mAssetCache == nullptr || materialCache.mMaterialParamsManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the material cache yet");

			return InvalidMaterialParamsPackHandle;
		}

		if (materialParamsPackManifestFilePath == nullptr || strlen(materialParamsPackManifestFilePath) == 0)
		{
			R2_CHECK(false, "Passed in an invalid materialParamsPackManifest File Path");
			return InvalidMaterialParamsPackHandle;
		}

		r2::asset::FileList fileList = materialCache.mAssetCache->GetFileList();

		r2::asset::Asset manifestAsset = r2::asset::Asset::MakeAssetFromFilePath(materialParamsPackManifestFilePath, r2::asset::MATERIAL_PACK_MANIFEST);

		const auto numMaterialParamPackManifests = r2::sarr::Capacity(*materialCache.mMaterialParamsManifests);
		bool found = false;
		s32 index = -1;

		for (u32 i = 0; i < numMaterialParamPackManifests; ++i)
		{
			const auto& entry = r2::sarr::At(*materialCache.mMaterialParamsManifests, i);
			if (entry.flatMaterialParamsPack == nullptr)
			{
				continue;
			}

			if (entry.assetCacheRecord.handle.handle == manifestAsset.HashID())
			{
				found = true;
				index = i;
				break;
			}
		}

		if (found)
		{
			return { manifestAsset.HashID(), materialCache.mName, index };
		}

		r2::asset::RawAssetFile* materialParamsPackManifestFile = r2::asset::lib::MakeRawAssetFile(materialParamsPackManifestFilePath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::MATERIAL_PACK_MANIFEST));

		r2::sarr::Push(*fileList, (r2::asset::AssetFile*)materialParamsPackManifestFile);

		MaterialParamsPackManifestEntry newEntry;

		r2::asset::AssetHandle assetHandle = materialCache.mAssetCache->LoadAsset(manifestAsset);

		if (r2::asset::IsInvalidAssetHandle(assetHandle))
		{
			R2_CHECK(false, "Failed to load the material params pack manifest file: %s", materialParamsPackManifestFilePath);
			return InvalidMaterialParamsPackHandle;
		}

		newEntry.assetCacheRecord = materialCache.mAssetCache->GetAssetBuffer(assetHandle);

		R2_CHECK(newEntry.assetCacheRecord.buffer != nullptr, "We should have a proper buffer here");

		newEntry.flatMaterialParamsPack = flat::GetMaterialParamsPack(newEntry.assetCacheRecord.buffer->Data());

		R2_CHECK(newEntry.flatMaterialParamsPack != nullptr, "We should have the flatbuffer data");

		MaterialParamsPackHandle newHandle = { manifestAsset.HashID(), materialCache.mName, -1 };

		for (u32 i = 0; i < numMaterialParamPackManifests; ++i)
		{
			const MaterialParamsPackManifestEntry& entry = r2::sarr::At(*materialCache.mMaterialParamsManifests, i);

			if (entry.flatMaterialParamsPack == nullptr)
			{
				r2::sarr::At(*materialCache.mMaterialParamsManifests, i) = newEntry;
				newHandle.packIndex = static_cast<s32>(i);
				break;
			}
		}

		return newHandle;
	}

	bool RemoveMaterialParamsPackFile(MaterialCache& materialCache, MaterialParamsPackHandle handle)
	{
		if (materialCache.mAssetCache == nullptr || materialCache.mMaterialParamsManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return false;
		}

		if (materialCache.mName != handle.cacheName)
		{
			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
			return false;
		}

		if (handle.packIndex < 0 || r2::sarr::Size(*materialCache.mMaterialParamsManifests) <= handle.packIndex)
		{
			R2_CHECK(false, "Handle: %i is not valid since it's not in the range 0 - %ull", handle.packIndex, r2::sarr::Capacity(*materialCache.mMaterialParamsManifests));
			return false;
		}

		auto& entry = r2::sarr::At(*materialCache.mMaterialParamsManifests, handle.packIndex);

		r2::asset::Asset manifestAsset = r2::asset::Asset(entry.assetCacheRecord.handle.handle, r2::asset::MATERIAL_PACK_MANIFEST);

		r2::asset::FileList fileList = materialCache.mAssetCache->GetFileList();

		const auto numFiles = r2::sarr::Size(*fileList);

		bool found = false;
		s32 foundIndex = -1;
		for (u32 i = 0; i < numFiles && !found; ++i)
		{
			const auto& file = r2::sarr::At(*fileList, i);
			const auto numAssets = file->NumAssets();

			for (u32 j = 0; j < numAssets && !found; ++j)
			{
				if (file->GetAssetHandle(j) == manifestAsset.HashID())
				{
					found = true;
					foundIndex = i;
				}
			}
		}

		if (found)
		{
			r2::sarr::RemoveAndSwapWithLastElement(*fileList, foundIndex);
		}

		materialCache.mAssetCache->ReturnAssetBuffer(entry.assetCacheRecord);
		entry.assetCacheRecord.buffer = nullptr;
		entry.assetCacheRecord.handle = {};
		entry.flatMaterialParamsPack = nullptr;

		return true;
	}

	const flat::MaterialParams* GetMaterialParamsForMaterialName(const MaterialCache& materialCache, const MaterialParamsPackHandle& materialCacheHandle, u64 materialName)
	{
		if (materialCache.mAssetCache == nullptr || materialCache.mMaterialParamsManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return false;
		}

		if (materialCache.mName != materialCacheHandle.cacheName)
		{
			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
			return false;
		}

		if (materialCacheHandle.packIndex < 0 || r2::sarr::Size(*materialCache.mMaterialParamsManifests) <= materialCacheHandle.packIndex)
		{
			R2_CHECK(false, "Handle: %i is not valid since it's not in the range 0 - %ull", materialCacheHandle.packIndex, r2::sarr::Capacity(*materialCache.mMaterialParamsManifests));
			return false;
		}

		const flat::MaterialParamsPack* materialParamsPack = r2::sarr::At(*materialCache.mMaterialParamsManifests, materialCacheHandle.packIndex).flatMaterialParamsPack;

		R2_CHECK(materialParamsPack != nullptr, "Should not be nullptr!");

		const auto numMaterialParams = materialParamsPack->pack()->size();

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(i);

			if (materialParams->name() == materialName)
			{
				return materialParams;
			}
		}

		return nullptr;
	}

	const flat::MaterialParams* GetMaterialParamsForMaterialHandle(const MaterialCache& materialCache, const MaterialParamsPackHandle& materialCacheHandle, const MaterialHandle& materialHandle)
	{
		if (materialCache.mAssetCache == nullptr || materialCache.mMaterialParamsManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return nullptr;
		}

		if (materialCache.mName != materialCacheHandle.cacheName)
		{
			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
			return nullptr;
		}

		if (materialCacheHandle.packIndex < 0 || r2::sarr::Size(*materialCache.mMaterialParamsManifests) <= materialCacheHandle.packIndex)
		{
			R2_CHECK(false, "Handle: %i is not valid since it's not in the range 0 - %ull", materialCacheHandle.packIndex, r2::sarr::Capacity(*materialCache.mMaterialParamsManifests));
			return nullptr;
		}

		const flat::MaterialParamsPack* materialParamsPack = r2::sarr::At(*materialCache.mMaterialParamsManifests, materialCacheHandle.packIndex).flatMaterialParamsPack;
		R2_CHECK(materialParamsPack != nullptr, "Should not be nullptr!");

		if (materialHandle.handle != materialParamsPack->name() || materialHandle.slot == -1 || materialHandle.slot >= materialParamsPack->pack()->size() )
		{
			R2_CHECK(false, "Invalid material handle passed in");
		}
		
		const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(materialHandle.slot);

		return materialParams;
	}

	MaterialHandle GetMaterialHandleForMaterialName(const MaterialCache& materialCache, const MaterialParamsPackHandle& materialCacheHandle, u64 materialName)
	{
		if (materialCache.mAssetCache == nullptr || materialCache.mMaterialParamsManifests == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the cache yet");
			return {};
		}

		if (materialCache.mName != materialCacheHandle.cacheName)
		{
			R2_CHECK(false, "Mismatching cache name between the cache and the handle");
			return {};
		}

		if (materialCacheHandle.packIndex < 0 || r2::sarr::Size(*materialCache.mMaterialParamsManifests) <= materialCacheHandle.packIndex)
		{
			R2_CHECK(false, "Handle: %i is not valid since it's not in the range 0 - %ull", materialCacheHandle.packIndex, r2::sarr::Capacity(*materialCache.mMaterialParamsManifests));
			return {};
		}

		const flat::MaterialParamsPack* materialParamsPack = r2::sarr::At(*materialCache.mMaterialParamsManifests, materialCacheHandle.packIndex).flatMaterialParamsPack;

		R2_CHECK(materialParamsPack != nullptr, "Should not be nullptr!");

		const auto numMaterialParams = materialParamsPack->pack()->size();

		for (flatbuffers::uoffset_t i = 0; i < numMaterialParams; ++i)
		{
			const flat::MaterialParams* materialParams = materialParamsPack->pack()->Get(i);

			if (materialParams->name() == materialName)
			{
				//@TODO(Serge): this is sort of incomplete now - need a way to make sure it belongs to a certain MaterialCache as well
				return { materialCacheHandle.packName, static_cast<s64>(i)};
			}
		}

		return {};
	}

	bool IsInvalidMaterialParamsPackHandle(const MaterialParamsPackHandle& handle)
	{
		return handle.cacheName != InvalidMaterialParamsPackHandle.cacheName ||  handle.packIndex != InvalidMaterialParamsPackHandle.packIndex || handle.packName != InvalidMaterialParamsPackHandle.packName;
	}

	bool AreMaterialParamsPackHandlesEqual(const MaterialParamsPackHandle& handle1, const MaterialParamsPackHandle& handle2)
	{
		return handle1.cacheName == handle2.cacheName && handle1.packIndex == handle2.packIndex && handle1.packName == handle2.packName;
	}
}