#ifndef __MANIFEST_ASSET_FILE_H__
#define __MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/AssetCacheRecord.h"

namespace r2::asset
{
	class AssetCache;

	class ManifestAssetFile
	{
	public:
		virtual ~ManifestAssetFile() {}
		virtual bool Init(const char* path, r2::asset::AssetType assetType) = 0;
		virtual void Shutdown() = 0;

		virtual r2::asset::AssetType GetAssetType() const = 0;
		virtual u64 GetManifestFileHandle() const = 0;
		
		virtual bool LoadManifest(AssetCache* assetCache) = 0;
		virtual bool UnloadManifest(AssetCache* assetCache) = 0;
		virtual const byte* GetManifestData() const = 0;
		virtual const char* FilePath() const = 0;
		
		//@NOTE(Serge): These shouldn't exist
		virtual bool AddAllFilePaths(FileList files) = 0;

#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, HotReloadType hotreloadType) = 0;
		using ReloadFilePathFunc = std::function<bool(const std::vector<std::string>&, const std::string& manifestFilePath, const byte* manifestData, HotReloadType hotreloadType)>;
		void SetReloadFilePathCallback(ReloadFilePathFunc func)
		{
			mReloadFilePathFunc = func;
		}

		virtual bool SaveManifest() const = 0;
		virtual void Reload(AssetCache* assetCache) = 0;
#endif
	protected:
#ifdef R2_ASSET_PIPELINE
		ReloadFilePathFunc mReloadFilePathFunc;
#endif
		AssetFile* mManifestAssetFile = nullptr;
		AssetCacheRecord mManifestCacheRecord;
		r2::asset::AssetType mAssetType;
		r2::asset::AssetHandle mManifestAssetHandle;
	};
}
#endif