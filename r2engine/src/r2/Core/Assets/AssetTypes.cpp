#include "r2pch.h"
#include "r2/Core/Assets/AssetTypes.h"

namespace r2::asset
{

	bool IsInvalidAssetHandle(const AssetHandle& assetHandle)
	{
		return assetHandle.assetCache == INVALID_ASSET_CACHE || assetHandle.handle == INVALID_ASSET_HANDLE;
	}

	bool AreAssetHandlesEqual(const AssetHandle& assetHandle1, const AssetHandle& assetHandle2)
	{
		return assetHandle1.assetCache == assetHandle2.assetCache && assetHandle1.handle == assetHandle2.handle;
	}
}