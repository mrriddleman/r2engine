#ifndef __RMODELS_MANIFEST_UTILS_H__
#define __RMODELS_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/AssetReference.h"
#include <vector>
#include <string>

namespace r2::asset::pln
{
	bool SaveAssetReferencesToManifestFile(u32 version, const std::vector<r2::asset::AssetReference>& assetReferences, const std::string& binFilePath, const std::string& rawFilePath);
}

#endif
#endif