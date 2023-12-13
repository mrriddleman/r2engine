#include "r2pch.h"

#include "r2/Core/Assets/AssetFiles/SoundAssetFile.h"
#include "r2/Audio/AudioEngine.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset
{

	SoundAssetFile::SoundAssetFile()
		:mIsOpen(false)
	{
		r2::util::PathCpy(mPath, "");
	}

	SoundAssetFile::~SoundAssetFile()
	{
		Close();
	}

	bool SoundAssetFile::Init(const char* path)
	{
		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(path, sanitizedPath);
		r2::util::PathCpy(mPath, sanitizedPath);

#ifdef R2_ASSET_PIPELINE
		r2::asset::MakeAssetNameStringForFilePath(sanitizedPath, mAssetName.assetNameString.data(), r2::asset::SOUND);
#endif
		mAssetName.hashID = r2::asset::GetAssetNameForFilePath(sanitizedPath, r2::asset::SOUND);


		return true;
	}

	bool SoundAssetFile::Open(bool writable /*= false*/)
	{
		mIsOpen = true;
		return mIsOpen;
	}

	bool SoundAssetFile::Open(r2::fs::FileMode mode)
	{
		mIsOpen = true;
		return mIsOpen;
	}

	bool SoundAssetFile::Close()
	{
		mIsOpen = false;
		return mIsOpen;
	}

	bool SoundAssetFile::IsOpen() const
	{
		return mIsOpen;
	}

	u64 SoundAssetFile::RawAssetSize(const r2::asset::Asset& asset)
	{
		return sizeof(r2::audio::AudioEngine::BankHandle);
	}

	u64 SoundAssetFile::LoadRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize)
	{
		return sizeof(r2::audio::AudioEngine::BankHandle);
	}

	u64 SoundAssetFile::WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset)
	{
		return 0;
	}

	u64 SoundAssetFile::NumAssets()
	{
		return 1;
	}

	void SoundAssetFile::GetAssetName(u64 index, char* name, u32 nameBuferSize)
	{
		strcpy(name, mAssetName.assetNameString.c_str());
	}

	u64 SoundAssetFile::GetAssetHandle(u64 index)
	{
		return mAssetName.hashID;
	}

}