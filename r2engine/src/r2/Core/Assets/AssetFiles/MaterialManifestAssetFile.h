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

		virtual bool Init(AssetCache* noptrAssetCache, const char* path, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType) override;
		virtual void Shutdown() override;

		virtual r2::asset::AssetType GetAssetType() const override;

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;
		virtual const byte* GetManifestData() const override;
		virtual const char* FilePath() const override;

		virtual bool AddAllFilePaths(FileList files) override;
		virtual u64 GetManifestFileHandle() const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, r2::asset::HotReloadType type) override;

		virtual bool SaveManifest() override;
		virtual void Reload() override;

		std::vector<r2::mat::Material>& GetMaterials();
#endif

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