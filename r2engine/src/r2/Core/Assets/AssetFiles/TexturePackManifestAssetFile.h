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

		virtual bool Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType) override;
		virtual r2::asset::AssetType GetAssetType() const override;
		virtual void Shutdown() override;

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;
		virtual const byte* GetManifestData() const override;


		virtual bool AddAllFilePaths(FileList files) override;
		virtual u64 GetManifestFileHandle() const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, r2::asset::HotReloadType type) override;

		virtual bool SaveManifest() override;
		virtual void Reload() override;
#endif

		virtual const char* FilePath() const override;

	private:
		const flat::TexturePacksManifest* mTexturePacksManifest;
		
	};
}

#endif // __TEXTURE_PACK_MANIFEST_ASSET_FILE_H__
