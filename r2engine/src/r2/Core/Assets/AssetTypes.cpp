#include "r2pch.h"
#include "r2/Core/Assets/AssetTypes.h"

namespace r2::asset
{
	u32 GetNumberOfParentDirectoriesToIncludeForAssetType(AssetType assetType)
	{
		switch (assetType)
		{
		case DEFAULT:
			return 0;
		case TEXTURE:
			return 2;
		case MODEL:
			return 1;
		case MESH:
			return 1;
		case CUBEMAP_TEXTURE:
			return 2;
		case MATERIAL_PACK_MANIFEST:
			return 0;
		case TEXTURE_PACK_MANIFEST:
			return 0;
		case RMODEL:
			return 0;
		case RANIMATION:
			return 0;
		case LEVEL:
			return 1;
		case LEVEL_PACK:
			return 0;
		case MATERIAL:
			return 0;
		case TEXTURE_PACK:
			return 0;
		case SOUND:
			return 1;
		default:

			//@TODO(Serge): may need to extend this using the Application
			R2_CHECK(false, "Unsupported assettype!");
			break;
		}

		return 0;
	}

	bool IsInvalidAssetHandle(const AssetHandle& assetHandle)
	{
		return assetHandle.assetCache == INVALID_ASSET_CACHE || assetHandle.handle == INVALID_ASSET_HANDLE;
	}

	bool AreAssetHandlesEqual(const AssetHandle& assetHandle1, const AssetHandle& assetHandle2)
	{
		return assetHandle1.assetCache == assetHandle2.assetCache && assetHandle1.handle == assetHandle2.handle;
	}
}