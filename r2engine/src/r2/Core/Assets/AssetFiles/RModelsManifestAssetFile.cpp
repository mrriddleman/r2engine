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

	bool RModelsManifestAssetFile::Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType)
	{
		mnoptrAssetCache = noptrAssetCache;
		mManifestAssetFile = (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(binPath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(assetType));
		mAssetType = assetType;
		r2::util::PathCpy(mRawPath, rawPath);
		r2::util::PathCpy(mWatchPath, watchPath);
		return mManifestAssetFile != nullptr;
	}

	void RModelsManifestAssetFile::Shutdown()
	{
		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord) ||
			!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle))
		{
			R2_CHECK(false, "We haven't unloaded the Manifest!");
			return;
		}
	}

	r2::asset::AssetType RModelsManifestAssetFile::GetManifestAssetType() const
	{
		return mAssetType;
	}

	u64 RModelsManifestAssetFile::GetManifestFileHandle() const
	{
		return r2::asset::GetAssetNameForFilePath(FilePath(), mAssetType);
	}

	bool RModelsManifestAssetFile::LoadManifest()
	{
		if (mnoptrAssetCache == nullptr)
		{
			R2_CHECK(false, "Passed in nullptr for the AssetCache");
			return false;
		}
		//@TODO(Serge): this breaks publish build since this method is only for asset pipeline
		bool foundAssetFile = mnoptrAssetCache->HasAsset(r2::asset::Asset(mManifestAssetFile->GetAssetHandle(0), mAssetType));//GetAssetFileForAssetHandle({ mManifestAssetFile->GetAssetHandle(0), mnoptrAssetCache->GetSlot() });

		if (!foundAssetFile)
		{
			//@Temporary(Serge): add it to the file list - remove when we do the AssetCache refactor
			FileList fileList = mnoptrAssetCache->GetFileList();
			r2::sarr::Push(*fileList, (AssetFile*)mManifestAssetFile);
		}

		mManifestAssetHandle = mnoptrAssetCache->LoadAsset(r2::asset::Asset::MakeAssetFromFilePath(FilePath(), mAssetType));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		bool success = !r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord);

		mRModelManifest = flat::GetRModelsManifest(mManifestCacheRecord.GetAssetBuffer()->Data());

#ifdef R2_ASSET_PIPELINE
		FillRModelsVector();
#endif

		return success;
	}

	bool RModelsManifestAssetFile::UnloadManifest()
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			return true;
		}

		bool success = mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
		R2_CHECK(success, "Failed to return the asset cache record");

		mManifestCacheRecord = {};
		mManifestAssetHandle = {};
		mRModelManifest = nullptr;
#ifdef R2_ASSET_PIPELINE
		mRModelAssetReferences.clear();
#endif
		return success;
	}

	const byte* RModelsManifestAssetFile::GetManifestData() const
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			R2_CHECK(false, "Probably a bug");
			return nullptr;
		}

		return mManifestCacheRecord.GetAssetBuffer()->Data();
	}

	const char* RModelsManifestAssetFile::FilePath() const
	{
		return mManifestAssetFile->FilePath();
	}

	bool RModelsManifestAssetFile::AddAllFilePaths(FileList files)
	{
		return true;
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

	bool RModelsManifestAssetFile::ReloadFilePath(const std::vector<std::string>& paths, pln::HotReloadType type)
	{
		if (!mReloadFilePathFunc)
		{
			return false;
		}
		return mReloadFilePathFunc(paths, FilePath(), GetManifestData(), type);
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
		if (!AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
			mManifestCacheRecord = {};
		}

		mManifestAssetHandle = mnoptrAssetCache->ReloadAsset(Asset::MakeAssetFromFilePath(FilePath(), GetManifestAssetType()));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		R2_CHECK(!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord), "Failed to get the asset cache record");

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