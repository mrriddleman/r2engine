#ifndef __MATERIAL_CACHE_H__
#define __MATERIAL_CACHE_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Render/Model/Materials/MaterialTypes.h"

namespace flat
{
	struct MaterialParamsPack;
	struct MaterialParams;
}

namespace r2::draw
{
	struct MaterialParamsPackManifestEntry
	{
		const flat::MaterialParamsPack* flatMaterialParamsPack;
		r2::asset::AssetCacheRecord assetCacheRecord;
	};

	struct MaterialCache
	{
		u64 mName;
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::utils::MemBoundary mAssetBoundary;

		r2::mem::StackArena* mArena;

		r2::SArray<MaterialParamsPackManifestEntry>* mMaterialParamsManifests;

		r2::asset::AssetCache* mAssetCache;

		static u64 MemorySize(u32 numMaterialParamsManifests, u32 cacheSize);
	};
}

namespace r2::draw::mtrlche
{
	struct MaterialParamsPackHandle
	{
		u64 packName = 0;
		u64 cacheName = 0;
		s32 packIndex = -1;
	};

	const MaterialParamsPackHandle InvalidMaterialParamsPackHandle = {};

	MaterialCache* Create(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numMaterialParamsManifests, u32 cacheSize, const char* areaName);
	void Shutdown(MaterialCache* materialCache);
	bool GetMaterialPacksCacheSizes(const char* materialParamsManifestFilePath, u32& numMaterials, u32& cacheSize);

	MaterialParamsPackHandle AddMaterialParamsPackFile(MaterialCache& materialCache, const char* materialParamsPackManifestFilePath);
	bool RemoveMaterialParamsPackFile(MaterialCache& materialCache, MaterialParamsPackHandle handle);

	const flat::MaterialParams* GetMaterialParamsForMaterialName(const MaterialCache& materialCache, const MaterialParamsPackHandle& materialCacheHandle, u64 materialName);
//	const flat::MaterialParams* GetMaterialParamsForMaterialHandle(const MaterialCache& materialCache, const MaterialParamsPackHandle& materialCacheHandle, const MaterialHandle& materialHandle);
//	MaterialHandle GetMaterialHandleForMaterialName(const MaterialCache& materialCache, const MaterialParamsPackHandle& materialCacheHandle, u64 materialName);

	bool IsInvalidMaterialParamsPackHandle(const MaterialParamsPackHandle& handle);
	bool AreMaterialParamsPackHandlesEqual(const MaterialParamsPackHandle& handle1, const MaterialParamsPackHandle& handle2);
}

#endif
