#ifndef __ASSET_TYPES_H__
#define __ASSET_TYPES_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include <functional>
#include "assetlib/AssetUtils.h"

namespace r2::asset
{
	const u64 INVALID_ASSET_HANDLE = 0;
	const s64 INVALID_ASSET_CACHE = -1;



	struct AssetName
	{
		//UUID uuid;
		u64 hashID;
#ifdef R2_ASSET_PIPELINE
		std::string assetNameString;
#endif

		bool operator==(const AssetName& otherAssetName) const;
	};

	struct AssetHandle
	{
		u64 handle = INVALID_ASSET_HANDLE;
		s64 assetCache = INVALID_ASSET_CACHE;

		bool operator==(const AssetHandle& assetHandle) const;
	};

	using AssetLoadProgressCallback = std::function<void(int, bool&)>;
	using AssetFreedCallback = std::function<void(const r2::asset::AssetHandle& handle)>;
	using AssetReloadedFunc = std::function<void(AssetHandle asset)>;

	class AssetFile;

	using FileList = r2::SArray<AssetFile*>*;

	using FileHandle = s64;



	bool IsInvalidAssetHandle(const AssetHandle& assetHandle);
	bool AreAssetHandlesEqual(const AssetHandle& assetHandle1, const AssetHandle& assetHandle2);
}

#endif // 
