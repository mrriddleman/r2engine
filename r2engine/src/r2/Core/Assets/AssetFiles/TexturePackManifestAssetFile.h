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
		virtual void Shutdown() override;

		virtual bool LoadManifest(AssetCache* assetCache) override;
		virtual bool UnloadManifest(AssetCache* assetCache) override;
		virtual const byte* GetManifestData() const override;


		virtual bool AddAllFilePaths(FileList files) override;
		virtual u64 GetManifestFileHandle() const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, r2::asset::HotReloadType type) override;

		virtual bool SaveManifest() const override;
		virtual void Reload(AssetCache* assetCache) override;
#endif

		virtual const char* FilePath() const override;

	private:
		const flat::TexturePacksManifest* mTexturePacksManifest;
		
	};
}

#endif // __TEXTURE_PACK_MANIFEST_ASSET_FILE_H__
