#ifndef __LEVEL_PACK_MANIFEST_ASSET_FILE_H__
#define __LEVEL_PACK_MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#include "r2/Core/Assets/Asset.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/AssetReference.h"
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#endif

namespace flat
{
	struct LevelPackData;
}

namespace r2::asset
{
	class LevelPackManifestAssetFile : public ManifestAssetFile
	{
	public:

		LevelPackManifestAssetFile();
		~LevelPackManifestAssetFile();

		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;

		//	virtual bool AddAllFilePaths(FileList files) override;
		virtual bool HasAsset(const Asset& asset) const override;
		virtual AssetFile* GetAssetFile(const Asset& asset) override;

#ifdef R2_ASSET_PIPELINE
		virtual bool AddAssetReference(const AssetReference& assetReference) override;
		virtual bool SaveManifest() override;
		virtual void Reload() override;
#endif

	protected:
		virtual void DestroyAssetFiles() override;

	private:
#ifdef R2_ASSET_PIPELINE
		void ReloadManifestFile(bool fillVector);
		void FillLevelPackVector();

		std::vector<pln::LevelGroup> mLevelPack;
#endif
		void FillLevelAssetFiles();

		const flat::LevelPackData* mLevelPackManifest;


	};
}


#endif