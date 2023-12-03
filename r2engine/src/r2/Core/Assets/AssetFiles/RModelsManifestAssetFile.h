#ifndef __RMODELS_MANIFEST_ASSET_FILE_H__
#define __RMODELS_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/AssetReference.h"
#endif

namespace flat
{
	struct RModelsManifest;
}

namespace r2::asset 
{
	class RModelsManifestAssetFile : public ManifestAssetFile
	{
	public:
		RModelsManifestAssetFile();
		virtual ~RModelsManifestAssetFile() {}

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;

		//@NOTE(Serge): These shouldn't exist
		virtual bool AddAllFilePaths(FileList files) override;

		virtual bool HasAsset(const Asset& asset) const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool AddAssetReference(const AssetReference& assetReference) override;

		virtual bool SaveManifest() override;
		virtual void Reload() override;

		std::vector<AssetReference>& GetRModelAssetReferences();
#endif

	private:
		const flat::RModelsManifest* mRModelManifest;

#ifdef R2_ASSET_PIPELINE
		void ReloadManifestFile(bool fillVector);
		void FillRModelsVector();

		std::vector<AssetReference> mRModelAssetReferences;
#endif
	};
}

#endif