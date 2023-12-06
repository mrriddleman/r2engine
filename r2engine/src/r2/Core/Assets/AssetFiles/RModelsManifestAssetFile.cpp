#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/RModelsManifestAssetFile.h"

#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"

#include "r2/Render/Model/RModelManifest_generated.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/RModelsManifestUtils.h"

#endif

namespace r2::asset
{
	RModelsManifestAssetFile::RModelsManifestAssetFile()
		:mRModelManifest(nullptr)
	{

	}

	bool RModelsManifestAssetFile::LoadManifest()
	{
		bool success = ManifestAssetFile::LoadManifest();

		mRModelManifest = flat::GetRModelsManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

#ifdef R2_ASSET_PIPELINE
		FillRModelsVector();
#endif

		return success && !mRModelManifest;
	}

	bool RModelsManifestAssetFile::UnloadManifest()
	{
		bool success = ManifestAssetFile::UnloadManifest();

		mRModelManifest = nullptr;
#ifdef R2_ASSET_PIPELINE
		mRModelAssetReferences.clear();
#endif
		return success;
	}

	//bool RModelsManifestAssetFile::AddAllFilePaths(FileList files)
	//{
	//	return true;
	//}

	bool RModelsManifestAssetFile::HasAsset(const Asset& asset) const
	{
		R2_CHECK(mRModelManifest != nullptr, "Should never happen");

		const auto* flatModelRefs = mRModelManifest->models();
		for (flatbuffers::uoffset_t i = 0; i < flatModelRefs->size(); ++i)
		{
			//@TODO(Serge): change this to UUID
			if (flatModelRefs->Get(i)->assetName()->assetName() == asset.HashID())
			{
				return true;
			}
		}

		return false;
	}

#ifdef R2_ASSET_PIPELINE

	bool RModelsManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		//first check to see if we have it already
		bool hasAssetReference = false;
		//if we don't add it
		for (u32 i = 0; i < mRModelAssetReferences.size(); ++i)
		{
			if (mRModelAssetReferences[i].assetName == assetReference.assetName)
			{
				hasAssetReference = true;
				break;
			}
		}

		if (!hasAssetReference)
		{
			mRModelAssetReferences.push_back(assetReference);

			//then regen the manifest
			return SaveManifest();
		}
		
		return hasAssetReference;
	}

	bool RModelsManifestAssetFile::SaveManifest()
	{
		r2::asset::pln::SaveRModelManifestAssetReferencesToManifestFile(1, mRModelAssetReferences, FilePath(), mRawPath);

		ReloadManifestFile(false);

		return mRModelManifest != nullptr;
	}

	void RModelsManifestAssetFile::Reload()
	{
		ReloadManifestFile(true);
	}

	std::vector<r2::asset::AssetReference>& RModelsManifestAssetFile::GetRModelAssetReferences()
	{
		return mRModelAssetReferences;
	}

	void RModelsManifestAssetFile::ReloadManifestFile(bool fillVector)
	{
		ManifestAssetFile::Reload();

		mRModelManifest = flat::GetRModelsManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		if (fillVector)
		{
			FillRModelsVector();
		}
	}

	void RModelsManifestAssetFile::FillRModelsVector()
	{
		mRModelAssetReferences.clear();
		R2_CHECK(mRModelManifest != nullptr, "Should never happen");
		const auto* flatRModelReferences = mRModelManifest->models();

		mRModelAssetReferences.reserve(flatRModelReferences->size());

		for (flatbuffers::uoffset_t i = 0; i < flatRModelReferences->size(); ++i)
		{
			const flat::AssetRef* flatAssetRef = flatRModelReferences->Get(i);

			AssetReference assetReference;
			MakeAssetReferenceFromFlatAssetRef(flatAssetRef, assetReference);
			mRModelAssetReferences.push_back(assetReference);
		}
	}
#endif
}