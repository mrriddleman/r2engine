#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/ManifestSingleAssetFile.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset
{
	ManifestSingleAssetFile::ManifestSingleAssetFile()
		:mFile(nullptr)
		,mNumDirectoriesToIncludeInAssetHandle(r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::MATERIAL_PACK_MANIFEST))
		,mAssetHandle(0)
	{
	}

	ManifestSingleAssetFile::~ManifestSingleAssetFile()
	{
		if (mFile)
		{
			Close();
		}
	}

	bool ManifestSingleAssetFile::Init(const char* path)
	{
		mAssetHandle = Asset::GetAssetNameForFilePath(path, GetAssetType());

		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(path, sanitizedPath);

		r2::util::PathCpy(mPath, sanitizedPath);

		return mAssetHandle != 0;
	}

	r2::asset::EngineAssetType ManifestSingleAssetFile::GetAssetType() const
	{
		return r2::asset::MATERIAL_PACK_MANIFEST;
	}

	bool ManifestSingleAssetFile::AddAllFilePaths(FileList files)
	{
		return true;
	}

	bool ManifestSingleAssetFile::HasFilePath(const char* filePath) const
	{
		return false;
	}

	u64 ManifestSingleAssetFile::GetManifestFileHandle() const
	{
		return mAssetHandle;
	}

	bool ManifestSingleAssetFile::ReloadFilePath(const char* filePath) const 
	{
		return false;
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
		r2::fs::utils::CopyFileNameWithParentDirectories(mPath, name, mNumDirectoriesToIncludeInAssetHandle);
	}

	u64 ManifestSingleAssetFile::GetAssetHandle(u64 index)
	{
		return mAssetHandle;
	}

	const char* ManifestSingleAssetFile::FilePath() const
	{
		return mPath;
	}
}