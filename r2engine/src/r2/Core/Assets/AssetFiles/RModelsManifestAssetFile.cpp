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

		FillAssetFiles();

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

	AssetFile* RModelsManifestAssetFile::GetAssetFile(const Asset& asset)
	{
		R2_CHECK(mAssetFiles != nullptr, "We should always have mAssetFiles set for this to work!");
		R2_CHECK(asset.GetType() == r2::asset::RMODEL, "This is the only type that is supported");

		for (u32 i = 0; i < r2::sarr::Size(*mAssetFiles); ++i)
		{
			AssetFile* assetFile = r2::sarr::At(*mAssetFiles, i);
			//@TODO(Serge): UUID
			if (assetFile->GetAssetHandle(0) == asset.HashID())
			{
				return assetFile;
			}
		}

		return nullptr;
	}

	void RModelsManifestAssetFile::DestroyAssetFiles()
	{
		s32 numFiles = r2::sarr::Size(*mAssetFiles);

		for (s32 i = numFiles - 1; i >= 0; --i)
		{
			RawAssetFile* rawAssetFile = (RawAssetFile*)(r2::sarr::At(*mAssetFiles, i));

			lib::FreeRawAssetFile(rawAssetFile);
		}

		lib::DestoryFileList(mAssetFiles);

		mAssetFiles = nullptr;
	}

	void RModelsManifestAssetFile::FillAssetFiles()
	{
		R2_CHECK(mAssetFiles == nullptr, "We shouldn't have any asset files yet");
		R2_CHECK(mRModelManifest != nullptr, "We haven't initialized the rmodel manifest!");
		u32 numModelFiles = mRModelManifest->models()->size();

#ifdef R2_ASSET_PIPELINE
		numModelFiles = glm::max(2000u, numModelFiles); //I dunno just make a lot so we don't run out
#endif

		mAssetFiles = lib::MakeFileList(numModelFiles);

		R2_CHECK(mAssetFiles != nullptr, "We couldn't create the asset files!");

		for (flatbuffers::uoffset_t i = 0; i < mRModelManifest->models()->size(); ++i)
		{
			r2::asset::RawAssetFile* modelFile = r2::asset::lib::MakeRawAssetFile(mRModelManifest->models()->Get(i)->binPath()->str().c_str(), r2::asset::RMODEL);

			r2::sarr::Push(*mAssetFiles, (AssetFile*)modelFile);
		}
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

	std::vector<r2::asset::AssetName> RModelsManifestAssetFile::GetAssetNames() const
	{
		std::vector<r2::asset::AssetName> assetNames = {};

		for (const auto& assetRef : mRModelAssetReferences)
		{
			assetNames.push_back(assetRef.assetName);
		}

		return assetNames;
	}

	void RModelsManifestAssetFile::ReloadManifestFile(bool fillVector)
	{
		ManifestAssetFile::Reload();

		mRModelManifest = flat::GetRModelsManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

		FillAssetFiles();

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