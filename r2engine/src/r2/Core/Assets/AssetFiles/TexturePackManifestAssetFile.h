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

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;

		virtual bool AddAllFilePaths(FileList files) override;
		virtual bool HasAsset(const Asset& asset) const override;

#ifdef R2_ASSET_PIPELINE

		virtual bool AddAssetReference(const AssetReference& assetReference) override;
		virtual bool SaveManifest() override;
		virtual void Reload() override;
#endif

	private:
		const flat::TexturePacksManifest* mTexturePacksManifest;
		
	};
}

#endif // __TEXTURE_PACK_MANIFEST_ASSET_FILE_H__
