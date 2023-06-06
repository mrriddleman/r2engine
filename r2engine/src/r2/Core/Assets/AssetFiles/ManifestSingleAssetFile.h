#ifndef __MATERIAL_MANIFEST_ASSET_FILE_H__
#define __MATERIAL_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"

namespace r2::fs
{
	class File;
}

namespace r2::asset
{
	class ManifestSingleAssetFile : public ManifestAssetFile
	{
	public:

		ManifestSingleAssetFile();
		~ManifestSingleAssetFile();

		virtual bool Init(const char* path) override;
		virtual r2::asset::EngineAssetType GetAssetType() override;
		virtual bool AddAllFilePaths(FileList files) override;
		virtual bool HasFilePath(const char* filePath) override;
		virtual u64 GetManifestFileHandle() override;

		virtual bool Open(bool writable = false) override;
		virtual bool Open(r2::fs::FileMode mode) override;
		virtual bool Close() override;
		virtual bool IsOpen() const override;
		virtual u64 RawAssetSize(const Asset& asset) override;
		virtual u64 LoadRawAsset(const Asset& asset, byte* data, u32 dataBufSize) override;
		virtual u64 WriteRawAsset(const Asset& asset, const byte* data, u32 dataBufferSize, u32 offset) override;
		virtual u64 NumAssets() override;
		virtual void GetAssetName(u64 index, char* name, u32 nameBuferSize) override;
		virtual u64 GetAssetHandle(u64 index) override;
		virtual const char* FilePath() const override;

	private:
		char mPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::File* mFile;
		u32 mNumDirectoriesToIncludeInAssetHandle;
		u64 mAssetHandle;
	};
}

#endif // __MATERIAL_MANIFEST_ASSET_FILE_H__
