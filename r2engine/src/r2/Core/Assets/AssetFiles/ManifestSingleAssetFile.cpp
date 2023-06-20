#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/ManifestSingleAssetFile.h"

#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset
{
	ManifestSingleAssetFile::ManifestSingleAssetFile()
		:mFile(nullptr)
		, mAsset{}
	{
		r2::util::PathCpy(mPath, "");
	}

	ManifestSingleAssetFile::~ManifestSingleAssetFile()
	{
		if (mFile)
		{
			Close();
		}
	}

	bool ManifestSingleAssetFile::Init(const char* path, r2::asset::AssetType assetType)
	{
		mAsset = r2::asset::Asset::MakeAssetFromFilePath(path, assetType);

		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(path, sanitizedPath);

		r2::util::PathCpy(mPath, sanitizedPath);

		return mAsset.HashID() != 0;
	}

	r2::asset::AssetType ManifestSingleAssetFile::GetAssetType() const
	{
		return mAsset.GetType();
	}

	bool ManifestSingleAssetFile::AddAllFilePaths(FileList files)
	{
		return true;
	}

	u64 ManifestSingleAssetFile::GetManifestFileHandle() const
	{
		return mAsset.HashID();
	}

#ifdef R2_ASSET_PIPELINE
	bool ManifestSingleAssetFile::ReloadFilePath(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, r2::asset::HotReloadType type)
	{
		return mReloadFilePathFunc(paths, manifestFilePath, manifestData, type);
	}
#endif

	bool ManifestSingleAssetFile::NeedsManifestData() const
	{
		return false;
	}

	void ManifestSingleAssetFile::SetManifestData(const byte* manifestData)
	{

	}

	bool ManifestSingleAssetFile::Open(bool writable /*= false*/)
	{
		r2::fs::FileMode mode;
		mode = r2::fs::Mode::Read;
		mode |= r2::fs::Mode::Binary;
		if (writable)
		{
			mode |= r2::fs::Mode::Write;
		}

		return Open(mode);
	}

	bool ManifestSingleAssetFile::Open(r2::fs::FileMode mode)
	{
		r2::fs::DeviceConfig config;
		mFile = r2::fs::FileSystem::Open(config, mPath, mode);
		return mFile != nullptr;
	}

	bool ManifestSingleAssetFile::Close()
	{
		r2::fs::FileSystem::Close(mFile);
		mFile = nullptr;
		return true;
	}

	bool ManifestSingleAssetFile::IsOpen() const
	{
		return mFile != nullptr;
	}

	u64 ManifestSingleAssetFile::RawAssetSize(const Asset& asset)
	{
		return mFile->Size();
	}

	u64 ManifestSingleAssetFile::LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize)
	{
		return mFile->ReadAll(data);
	}

	u64 ManifestSingleAssetFile::WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset)
	{
		mFile->Seek(offset);
		return mFile->Write(data, dataBufferSize);
	}

	u64 ManifestSingleAssetFile::NumAssets()
	{
		return 1;
	}

	void ManifestSingleAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize)
	{
		r2::fs::utils::CopyFileNameWithParentDirectories(mPath, name, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(mAsset.GetType()));
	}

	u64 ManifestSingleAssetFile::GetAssetHandle(u64 index)
	{
		return mAsset.HashID();
	}

	const char* ManifestSingleAssetFile::FilePath() const
	{
		return mPath;
	}
}