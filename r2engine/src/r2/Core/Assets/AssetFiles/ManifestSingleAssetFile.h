#ifndef __MANIFEST_SINGLE_ASSET_FILE_H__
#define __MANIFEST_SINGLE_ASSET_FILE_H__

#include "r2/Core/Assets/AssetFiles/ManifestAssetFile.h"
#include "r2/Core/Assets/Asset.h"


namespace r2::asset
{
	class ManifestSingleAssetFile : public ManifestAssetFile
	{
	public:
		ManifestSingleAssetFile();
		~ManifestSingleAssetFile();

		virtual bool Init(AssetCache* noptrAssetCache, const char* binpath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType) override;
		virtual void Shutdown() override;

		virtual r2::asset::AssetType GetAssetType() const override;


		virtual bool LoadManifest() override;
		virtual bool UnloadManifest() override;
		virtual const byte* GetManifestData() const override;

		virtual bool AddAllFilePaths(FileList files) override;
		virtual u64 GetManifestFileHandle() const override;
#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, pln::HotReloadType type) override;

		virtual bool SaveManifest() override;
		virtual void Reload() override;
#endif

		virtual const char* FilePath() const override;


	private:
		
	};
}

#endif // __MATERIAL_MANIFEST_ASSET_FILE_H__
