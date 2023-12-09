#ifndef __ASSET_REFERENCE_H__
#define __ASSET_REFERENCE_H__
#ifdef R2_ASSET_PIPELINE

namespace flat
{
	struct AssetRef;
}

#include "r2/Core/Assets/AssetTypes.h"
#include <filesystem>

namespace r2::asset
{
	struct AssetReference
	{
		AssetName assetName;
		std::filesystem::path binPath;
		std::filesystem::path rawPath;
	};

	struct AssetReferenceAndType
	{
		AssetReference assetReference;
		r2::asset::AssetType type;
	};

	void MakeAssetReferenceFromFlatAssetRef(const flat::AssetRef* flatAssetRef, AssetReference& outAssetReference);
	AssetReference CreateNewAssetReference(const std::filesystem::path& binPath, const std::filesystem::path& rawPath, r2::asset::AssetType assetType);
}

#endif
#endif