#include "r2pch.h"
#include "r2/Core/Assets/AssetFiles/MaterialManifestAssetFile.h"
#include "r2/Core/Assets/Asset.h"
#include "r2/Core/File/File.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset
{
	SingleManifestAssetFile::SingleManifestAssetFile()
		:mFile(nullptr)
		,mNumDirectoriesToIncludeInAssetHandle(r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::MATERIAL_PACK_MANIFEST))
		,mAssetHandle(0)
	{
	}

	SingleManifestAssetFile::~SingleManifestAssetFile()
	{
		if (mFile)
		{
			Close();
		}
	}

	bool SingleManifestAssetFile::Init(const char* path)
	{
		mAssetHandle = Asset::GetAssetNameForFilePath(path, GetAssetType());

		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(path, sanitizedPath);

		r2::util::PathCpy(mPath, sanitizedPath);

		return mAssetHandle != 0;
	}

	r2::asset::EngineAssetType SingleManifestAssetFile::GetAssetType()
	{
		return r2::asset::MATERIAL_PACK_MANIFEST;
	}

	bool SingleManifestAssetFile::AddAllFilePaths(FileList files)
	{
		return true;
	}

	bool SingleManifestAssetFile::HasFilePath(const char* filePath)
	{
		return false;
	}

	u64 SingleManifestAssetFile::GetManifestFileHandle()
	{
		return mAssetHandle;
	}

	bool SingleManifestAssetFile::Open(bool writable /*= false*/)
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

	bool SingleManifestAssetFile::Open(r2::fs::FileMode mode)
	{
		r2::fs::DeviceConfig config;
		mFile = r2::fs::FileSystem::Open(config, mPath, mode);
		return mFile != nullptr;
	}

	bool SingleManifestAssetFile::Close()
	{
		r2::fs::FileSystem::Close(mFile);
		mFile = nullptr;
		return true;
	}

	bool SingleManifestAssetFile::IsOpen() const
	{
		return mFile != nullptr;
	}

	u64 SingleManifestAssetFile::RawAssetSize(const Asset& asset)
	{
		return mFile->Size();
	}

	u64 SingleManifestAssetFile::LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize)
	{
		return mFile->ReadAll(data);
	}

	u64 SingleManifestAssetFile::WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset)
	{
		mFile->Seek(offset);
		return mFile->Write(data, dataBufferSize);
	}

	u64 SingleManifestAssetFile::NumAssets()
	{
		return 1;
	}

	void SingleManifestAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize)
	{
		r2::fs::utils::CopyFileNameWithParentDirectories(mPath, name, mNumDirectoriesToIncludeInAssetHandle);
	}

	u64 SingleManifestAssetFile::GetAssetHandle(u64 index)
	{
		return mAssetHandle;
	}

	const char* SingleManifestAssetFile::FilePath() const
	{
		return mPath;
	}
}