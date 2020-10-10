#ifndef __ASSET_TYPES_H__
#define __ASSET_TYPES_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::asset
{
	class AssetFile;

	using FileList = r2::SArray<AssetFile*>*;
	const u64 INVALID_ASSET_HANDLE = 0;
	const s64 INVALID_ASSET_CACHE = -1;

	struct AssetHandle
	{
		u64 handle = INVALID_ASSET_HANDLE;
		s64 assetCache = INVALID_ASSET_CACHE;
	};

	using FileHandle = s64;

	using AssetType = u32;

	enum EngineAssetType : u32
	{
		DEFAULT = 0,
		TEXTURE,
		MODEL,
		ASSIMP_MODEL,
		ASSIMP_ANIMATION,
		NUM_ENGINE_ASSET_TYPES
	};
}

#endif // 