#ifndef __TEXTURE_PACK_MANIFEST_ASSET_FILE_H__
#define __TEXTURE_PACK_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#include "r2/Core/Assets/Asset.h"

namespace flat
{
	struct TexturePacksManifest;
}

namespace r2::fs
{
	class File;
}

namespace r2::asset
{
	class TexturePackManifestAssetFile : public ManifestAssetFile
	{
	public:

		TexturePackManifestAssetFile();
		~TexturePackManifestAssetFile();

		virtual bool Init(const char* path, r2::asset::AssetType assetType) override;
		virtual r2::asset::AssetType GetAssetType() const override;
		virtual bool AddAllFilePaths(FileList files) override;
		virtual u64 GetManifestFileHandle() const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, const byte* manifestData, r2::asset::HotReloadType type) override;
#endif
		virtual bool NeedsManifestData() const override;
		virtual void SetManifestData(const byte* manifestData) override;

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
		r2::asset::Asset mAsset;
		const flat::TexturePacksManifest* mTexturePacksManifest;
		
	};
}

#endif // __TEXTURE_PACK_MANIFEST_ASSET_FILE_H__
