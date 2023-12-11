#ifndef __MATERIAL_MANIFEST_ASSET_FILE_H__
#define __MATERIAL_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Render/Model/Materials/Material.h"
#endif

namespace flat
{
	struct MaterialPack;
}

namespace r2::asset
{
	class MaterialManifestAssetFile : public ManifestAssetFile
	{
	public:
		MaterialManifestAssetFile();
		~MaterialManifestAssetFile();

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;

		virtual bool HasAsset(const Asset& asset) const override;
		virtual AssetFile* GetAssetFile(const Asset& asset) override;

#ifdef R2_ASSET_PIPELINE

		virtual bool AddAssetReference(const AssetReference& assetReference) override;
		virtual bool SaveManifest() override;
		virtual void Reload() override;

		std::vector<r2::mat::Material>& GetMaterials();
#endif
	protected:
		virtual void DestroyAssetFiles() override;

	private:

		const flat::MaterialPack* mMaterialPackManifest;

#ifdef R2_ASSET_PIPELINE
		void ReloadManifestFile(bool fillVector);
		void FillMaterialVector();

		std::vector<r2::mat::Material> mMaterials;
#endif
	};
}

#endif