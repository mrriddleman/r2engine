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

}

#endif