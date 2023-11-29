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
		virtual bool Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType) override;
		virtual void Shutdown() override;

		virtual r2::asset::AssetType GetManifestAssetType() const override;
		virtual u64 GetManifestFileHandle() const override;

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;
		virtual const byte* GetManifestData() const override;
		virtual const char* FilePath() const override;

		//@NOTE(Serge): These shouldn't exist
		virtual bool AddAllFilePaths(FileList files) override;

		virtual bool HasAsset(const Asset& asset) const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool AddAssetReference(const AssetReference& assetReference) override;
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, pln::HotReloadType type) override;

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