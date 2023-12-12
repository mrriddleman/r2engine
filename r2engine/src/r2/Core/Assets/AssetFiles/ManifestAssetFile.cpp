#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Core/Assets/AssetCache.h"

namespace r2::asset
{
	 
	bool ManifestAssetFile::Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType)
	{
		mnoptrAssetCache = noptrAssetCache;
		mManifestAssetFile = (r2::asset::AssetFile*)r2::asset::lib::MakeRawAssetFile(binPath, assetType);
		mAssetType = assetType;

		r2::util::PathCpy(mRawPath, rawPath);
		r2::util::PathCpy(mWatchPath, watchPath);

		return mManifestAssetFile != nullptr;
	}

	void ManifestAssetFile::Shutdown()
	{
		if (mAssetFiles != nullptr)
		{
			DestroyAssetFiles();
		}

		if (mManifestAssetFile)
		{
			lib::FreeRawAssetFile((r2::asset::RawAssetFile*)mManifestAssetFile);
		}

		if (!r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord) ||
			!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle))
		{
			R2_CHECK(false, "We haven't unloaded the Manifest!");
			return;
		}
	}

	r2::asset::AssetType ManifestAssetFile::GetManifestAssetType() const
	{
		return mAssetType;
	}

	u64 ManifestAssetFile::GetManifestFileHandle() const
	{
		return mManifestAssetFile->GetAssetHandle(0);
	}

	bool ManifestAssetFile::LoadManifest()
	{
		if (mnoptrAssetCache == nullptr)
		{
			R2_CHECK(false, "Passed in nullptr for the AssetCache");
			return false;
		}

		mManifestAssetHandle = mnoptrAssetCache->LoadAsset(r2::asset::Asset::MakeAssetFromFilePath(FilePath(), mAssetType));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(mManifestAssetHandle), "The assetHandle for %s is invalid!\n", FilePath());

		mManifestCacheRecord = mnoptrAssetCache->GetAssetBuffer(mManifestAssetHandle);

		return !r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord);
	}

	bool ManifestAssetFile::UnloadManifest()
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			return true;
		}

		bool success = mnoptrAssetCache->ReturnAssetBuffer(mManifestCacheRecord);
		R2_CHECK(success, "Failed to return the asset cache record");

		mManifestCacheRecord = {};
		mManifestAssetHandle = {};

		if (mAssetFiles != nullptr)
		{
			DestroyAssetFiles();
		}
		

		return success;
	}

	const byte* ManifestAssetFile::GetManifestData() const
	{
		if (r2::asset::AssetCacheRecord::IsEmptyAssetCacheRecord(mManifestCacheRecord))
		{
			R2_CHECK(false, "Probably a bug");
			return nullptr;
		}

		return mManifestCacheRecord.GetAssetBuffer()->Data();
	}

	const char* ManifestAssetFile::FilePath() const
	{
		return mManifestAssetFile->FilePath();
	}

	bool ManifestAssetFile::HasAsset(const Asset& asset) const
	{
		return false;
	}

	r2::asset::AssetFile* ManifestAssetFile::GetAssetFile(const Asset& asset)
	{
		return nullptr;
	}

	AssetFile* ManifestAssetFile::GetManifestAssetFile() const
	{
		return mManifestAssetFile;
	}

#ifdef R2_ASSET_PIPELINE
	bool ManifestAssetFile::AddAssetReference(const AssetReference& assetReference)
	{
		return false;
	}

	bool ManifestAssetFile::ReloadFilePath(const std::vector<std::string>& paths, pln::HotReloadType hotreloadType)
	{
		if (!mReloadFilePathFunc)
		{
			return false;
		}

		return mReloadFilePathFunc(paths, FilePath(), GetManifestData(), hotreloadType);
	}

	bool ManifestAssetFile::SaveManifest()
	{
		return false;
	}

	void ManifestAssetFile::Reload()
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

		DestroyAssetFiles();
	}

	

#endif

}