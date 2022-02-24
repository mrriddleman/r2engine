#include "r2pch.h"
#include "GLTFAssetFile.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"

namespace r2::asset
{
	GLTFAssetFile::GLTFAssetFile() : mFile(nullptr), mNumDirectoriesToIncludeInAssetHandle(0), mSize(0)
	{
		r2::util::PathCpy(mPath, "");
		r2::util::PathCpy(mBinPath, "");
		
	}

	GLTFAssetFile::~GLTFAssetFile()
	{
		if (mFile)
		{
			Close();
		}
	}

	bool GLTFAssetFile::Init(const char* path, u32 numDirectoriesToIncludeInAssetHandle)
	{
		if (!path)
		{
			return false;
		}
		mNumDirectoriesToIncludeInAssetHandle = numDirectoriesToIncludeInAssetHandle;
		r2::util::PathCpy(mPath, path);
		r2::util::PathCpy(mBinPath, path);
		
		r2::fs::utils::SetNewExtension(mBinPath, ".bin");

		char fileName[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::CopyFileNameWithParentDirectories(mPath, fileName, mNumDirectoriesToIncludeInAssetHandle);

		std::transform(std::begin(fileName), std::end(fileName), std::begin(fileName), (int(*)(int))std::tolower);

		mAssetHandle = STRING_ID(fileName);


		return strcmp(mPath, "") != 0;
	}

	bool GLTFAssetFile::Open()
	{
		//@NOTE(Serge): We don't open the file ourselves right now, we let assimp do that for us
		return true;
	}

	bool GLTFAssetFile::Close()
	{
		return true;
	}

	bool GLTFAssetFile::IsOpen() const
	{
		//@NOTE(Serge): We don't open the file ourselves right now, we let assimp do that for us, we pretend it's always open
		return true;
	}

	u64 GLTFAssetFile::RawAssetSize(const r2::asset::Asset& asset)
	{
		r2::fs::DeviceConfig config;
		r2::fs::FileMode mode;
		mode = r2::fs::Mode::Read;
		mode |= r2::fs::Mode::Binary;

		mFile = r2::fs::FileSystem::Open(config, mPath, mode);

		R2_CHECK(mFile != nullptr, "File couldn't be opened!");

		mSize = mFile->Size();

		r2::fs::FileSystem::Close(mFile);

		mFile = r2::fs::FileSystem::Open(config, mBinPath, mode);

		if (mFile != nullptr)
		{
			mSize += mFile->Size();
			r2::fs::FileSystem::Close(mFile);
		}

		return mSize;
	}

	u64 GLTFAssetFile::GetRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize)
	{
		return mSize;
	}

	u64 GLTFAssetFile::NumAssets() const
	{
		return 1;
	}

	void GLTFAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize) const
	{
		r2::fs::utils::CopyFileNameWithParentDirectories(mPath, name, mNumDirectoriesToIncludeInAssetHandle);
	}

	u64 GLTFAssetFile::GetAssetHandle(u64 index) const
	{
		return mAssetHandle;
	}
}