#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/ManifestSingleAssetFile.h"

#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"

namespace r2::asset
{
	ManifestSingleAssetFile::ManifestSingleAssetFile()
	{

	}

	ManifestSingleAssetFile::~ManifestSingleAssetFile()
	{
	}

	bool ManifestSingleAssetFile::Init(const char* path, r2::asset::AssetType assetType)
	{
		mManifestAssetFile = (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(path, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(assetType));
		mAssetType = assetType;

		return mManifestAssetFile != nullptr;
	}

	void ManifestSingleAssetFile::Shutdown()
	{
		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord) ||
			!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle))
		{
			R2_CHECK(false, "We haven't unloaded the Manifest!");
			return;
		}
	}

	r2::asset::AssetType ManifestSingleAssetFile::GetAssetType() const
	{
		return mAssetType;
	}

	bool ManifestSingleAssetFile::AddAllFilePaths(FileList files)
	{
		return true;
	}

	u64 ManifestSingleAssetFile::GetManifestFileHandle() const
	{
		return r2::asset::GetAssetNameForFilePath(FilePath(), mAssetType);
	}

#ifdef R2_ASSET_PIPELINE
	bool ManifestSingleAssetFile::ReloadFilePath(const std::vector<std::string>& paths, r2::asset::HotReloadType type)
	{
		return mReloadFilePathFunc(paths, FilePath(), GetManifestData(), type);
	}
#endif

	const char* ManifestSingleAssetFile::FilePath() const
	{
		return mManifestAssetFile->FilePath();
	}

	bool ManifestSingleAssetFile::LoadManifest(AssetCache* assetCache)
	{
		if (assetCache == nullptr)
		{
			R2_CHECK(false, "Passed in nullptr for the AssetCache");
			return false;
		}

		const r2::asset::AssetFile* foundAssetFile = assetCache->GetAssetFileForAssetHandle({ mManifestAssetFile->GetAssetHandle(0), assetCache->GetSlot() });

		if (foundAssetFile != mManifestAssetFile)
		{
			//@Temporary(Serge): add it to the file list - remove when we do the AssetCache refactor
			FileList fileList = assetCache->GetFileList();
			r2::sarr::Push(*fileList, (AssetFile*)mManifestAssetFile);
		}

		mManifestAssetHandle = assetCache->LoadAsset(r2::asset::Asset::MakeAssetFromFilePath(FilePath(), mAssetType));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = assetCache->GetAssetBuffer(mManifestAssetHandle);

		return !r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord);
	}

	bool ManifestSingleAssetFile::UnloadManifest(AssetCache* assetCache)
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			return true;
		}

		bool success = assetCache->ReturnAssetBuffer(mManifestCacheRecord);
		R2_CHECK(success, "Failed to return the asset cache record");

		mManifestCacheRecord = {};
		mManifestAssetHandle = {};

		return success;
	}

	const byte* ManifestSingleAssetFile::GetManifestData() const
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			R2_CHECK(false, "Probably a bug");
			return nullptr;
		}

		return mManifestCacheRecord.GetAssetBuffer()->Data();
	}

#ifdef R2_ASSET_PIPELINE
	bool ManifestSingleAssetFile::SaveManifest() const
	{
		TODO;
		return false;
	}

	void ManifestSingleAssetFile::Reload(AssetCache* assetCache)
	{
		if (!AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			assetCache->ReturnAssetBuffer(mManifestCacheRecord);
			mManifestCacheRecord = {};
		}

		mManifestAssetHandle = assetCache->ReloadAsset(Asset::MakeAssetFromFilePath(FilePath(), GetAssetType()));
		
		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = assetCache->GetAssetBuffer(mManifestAssetHandle);

		R2_CHECK(!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord), "Failed to get the asset cache record");
	}
#endif
}