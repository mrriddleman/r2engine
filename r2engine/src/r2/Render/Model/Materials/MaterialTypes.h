#ifndef __MATERIAL_TYPES_H__
#define __MATERIAL_TYPES_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Assets/AssetTypes.h"

namespace r2::mat
{
	struct MaterialName
	{
		r2::asset::AssetName assetName;
		r2::asset::AssetName packName;

		bool operator==(const MaterialName& materialName) const
		{
			return assetName == materialName.assetName && packName == materialName.packName;
		}

		bool operator<(const MaterialName& materialName) const
		{
			return packName.hashID < materialName.packName.hashID || (assetName.hashID < materialName.assetName.hashID&& packName == materialName.packName);
		}

		bool IsInavlid() const
		{
			return assetName.hashID == 0 || packName.hashID == 0;
		}
	};
}

#endif // __MATERIAL_TYPES_H__
