#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/AssetReference.h"
#include "r2/Core/Assets/AssetRef_generated.h"
namespace r2::asset
{

	void MakeAssetReferenceFromFlatAssetRef(const flat::AssetRef* flatAssetRef, AssetReference& outAssetReference)
	{
		outAssetReference.assetName.assetNameString = flatAssetRef->assetName()->stringName()->str();
		outAssetReference.assetName.hashID = flatAssetRef->assetName()->assetName();
		//@TODO(Serge): read the UUID
		outAssetReference.binPath = flatAssetRef->binPath()->str();
		outAssetReference.rawPath = flatAssetRef->rawPath()->str();
	}

	AssetReference CreateNewAssetReference(const std::filesystem::path& binPath, const std::filesystem::path& rawPath, r2::asset::AssetType assetType)
	{
		AssetReference newAssetReference;

		newAssetReference.binPath = binPath;
		newAssetReference.rawPath = rawPath;
		newAssetReference.assetName.hashID = r2::asset::GetAssetNameForFilePath(binPath.string().c_str(), assetType);
		char nameStr[r2::fs::FILE_PATH_LENGTH];
		r2::asset::MakeAssetNameStringForFilePath(binPath.string().c_str(), nameStr, assetType);
		newAssetReference.assetName.assetNameString = nameStr;

		return newAssetReference;
	}

}

#endif