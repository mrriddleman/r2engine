#include "r2pch.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetName_generated.h"

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

	bool AssetHandle::operator==(const AssetHandle& assetHandle) const
	{
		return AreAssetHandlesEqual(*this, assetHandle);
	}

	bool AssetName::operator==(const AssetName& otherAssetName) const
	{
		return hashID == otherAssetName.hashID
			//@TODO(Serge): add UUID
//#ifdef R2_ASSET_PIPELINE
//			&& assetNameString == otherAssetName.assetNameString
//#endif
			;
	}

	void MakeAssetNameFromFlatAssetName(const flat::AssetName* flatAssetName, AssetName& outAssetName)
	{
		//@TODO(Serge): UUID
		outAssetName.hashID = flatAssetName->assetName();
#ifdef R2_ASSET_PIPELINE
		outAssetName.assetNameString = flatAssetName->stringName()->str();
#endif
	}

}