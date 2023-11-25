#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/ManifestSingleAssetFile.h"

#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetCache.h"

namespace r2::asset
{
	ManifestSingleAssetFile::ManifestSingleAssetFile()
	{

	}

	ManifestSingleAssetFile::~ManifestSingleAssetFile()
	{
	}

	bool ManifestSingleAssetFile::Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType)
	{
		mnoptrAssetCache = noptrAssetCache;
		mManifestAssetFile = (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(binPath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(assetType));
		mAssetType = assetType;
		r2::util::PathCpy(mRawPath, rawPath);
		r2::util::PathCpy(mWatchPath, watchPath);
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
	bool ManifestSingleAssetFile::ReloadFilePath(const std::vector<std::string>& paths, pln::HotReloadType type)
	{
		return mReloadFilePathFunc(paths, FilePath(), GetManifestData(), type);
	}
#endif

	const char* ManifestSingleAssetFile::FilePath() const
	{
		return mManifestAssetFile->FilePath();
	}

	bool ManifestSingleAssetFile::LoadManifest()
	{
		if (mnoptrAssetCache == nullptr)
		{
			R2_CHECK(false, "Passed in nullptr for the AssetCache");
			return false;
		}

		const r2::asset::AssetFile* foundAssetFile = mnoptrAssetCache->GetAssetFileForAssetHandle({ mManifestAssetFile->GetAssetHandle(0), mnoptrAssetCache->GetSlot() });

		if (foundAssetFile != mManifestAssetFile)
		{
			//@Temporary(Serge): add it to the file list - remove when we do the AssetCache refactor
			FileList fileList = mnoptrAssetCache->GetFileList();
			r2::sarr::Push(*fileList, (AssetFile*)mManifestAssetFile);
		}

		mManifestAssetHandle = mnoptrAssetCache->LoadAsset(r2::asset::Asset::MakeAssetFromFilePath(FilePath(), mAssetType));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		return !r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord);
	}

	bool ManifestSingleAssetFile::UnloadManifest()
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			return true;
		}

		bool success = mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
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
	bool ManifestSingleAssetFile::SaveManifest()
	{
		TODO;
		return false;
	}

	void ManifestSingleAssetFile::Reload()
	{
		if (!AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
			mManifestCacheRecord = {};
		}

		mManifestAssetHandle = mnoptrAssetCache->ReloadAsset(Asset::MakeAssetFromFilePath(FilePath(), GetAssetType()));
		
		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		R2_CHECK(!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord), "Failed to get the asset cache record");
	}
#endif
}