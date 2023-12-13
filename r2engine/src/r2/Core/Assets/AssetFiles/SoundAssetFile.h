//
//  RawAssetFile.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-07-17.
//

#ifndef __SOUND_ASSET_FILE_H__
#define __SOUND_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/AssetTypes.h"

namespace r2::asset
{
	class SoundAssetFile : public AssetFile {
	public:
		SoundAssetFile();
		~SoundAssetFile();
		bool Init(const char* path);
		virtual bool Open(bool writable = false) override;
		virtual bool Open(r2::fs::FileMode mode) override;
		virtual bool Close() override;
		virtual bool IsOpen() const override;
		virtual u64 RawAssetSize(const r2::asset::Asset& asset) override;
		virtual u64 LoadRawAsset(const r2::asset::Asset& asset, byte* data, u32 dataBufSize) override;
		virtual u64 WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset) override;
		virtual u64 NumAssets() override;
		virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) override;
		virtual u64 GetAssetHandle(u64 index) override;
		virtual const char* FilePath() const override { return mPath; }
	private:
		char mPath[r2::fs::FILE_PATH_LENGTH];
		AssetName mAssetName;
		bool mIsOpen;
	};
}

#endif /* __SOUND_ASSET_FILE_H__ */
