#ifndef __ASSET_TYPES_H__
#define __ASSET_TYPES_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include <functional>

namespace r2::asset
{
	const u64 INVALID_ASSET_HANDLE = 0;
	const s64 INVALID_ASSET_CACHE = -1;

	struct AssetHandle
	{
		u64 handle = INVALID_ASSET_HANDLE;
		s64 assetCache = INVALID_ASSET_CACHE;
	};

	using AssetLoadProgressCallback = std::function<void(int, bool&)>;
	using AssetFreedCallback = std::function<void(const r2::asset::AssetHandle& handle)>;
	using AssetReloadedFunc = std::function<void(AssetHandle asset)>;

	class AssetFile;

	using FileList = r2::SArray<AssetFile*>*;


	

	using FileHandle = s64;

	using AssetType = u32;

	enum EngineAssetType : u32
	{
		DEFAULT = 0,
		TEXTURE,
		MODEL,
		MESH,
		ASSIMP_MODEL,
		ASSIMP_ANIMATION,
		CUBEMAP_TEXTURE,
		GLTF_MODEL,
		MATERIAL_PACK_MANIFEST,
		TEXTURE_PACK_MANIFEST,
		RMODEL,
		RANIMATION,
		LEVEL,
		LEVEL_PACK,
		NUM_ENGINE_ASSET_TYPES
	};

	
	u32 GetNumberOfParentDirectoriesToIncludeForAssetType(AssetType assetType);


	bool IsInvalidAssetHandle(const AssetHandle& assetHandle);
	bool AreAssetHandlesEqual(const AssetHandle& assetHandle1, const AssetHandle& assetHandle2);
}

#endif // 
