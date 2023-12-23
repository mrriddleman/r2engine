#ifndef __ENGINE_MODEL_MANIFEST_ASSET_FILE_H__
#define __ENGINE_MODEL_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#include "r2/Core/Assets/Asset.h"

#ifdef R2_ASSET_PIPELINE
namespace r2::asset
{
	struct AssetReference;
}
#endif

namespace flat
{
	struct ModelsManifest;
}

namespace r2::asset 
{
	class EngineModelManifestAssetFile : public ManifestAssetFile
	{
	public:

		EngineModelManifestAssetFile();
		~EngineModelManifestAssetFile();

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;

		virtual bool HasAsset(const Asset& asset) const override;

		virtual AssetFile* GetAssetFile(const Asset& asset) override;

#ifdef R2_ASSET_PIPELINE
		virtual bool AddAssetReference(const AssetReference& assetReference) override;
		virtual bool SaveManifest() override;
		virtual void Reload() override;

		virtual std::vector<r2::asset::AssetName> GetAssetNames() const override;
#endif
	protected:
		virtual void DestroyAssetFiles() override;

	private:

		void FillAssetFiles();

		const flat::ModelsManifest* mModelsManifest;


	};
}

#endif