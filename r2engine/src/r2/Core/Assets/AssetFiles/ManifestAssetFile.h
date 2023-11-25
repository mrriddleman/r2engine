#ifndef __MANIFEST_ASSET_FILE_H__
#define __MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Core/Assets/AssetCacheRecord.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommandTypes.h"
#endif
namespace r2::asset
{
	class AssetCache;

	class ManifestAssetFile
	{
	public:
		virtual ~ManifestAssetFile() {}
		virtual bool Init(AssetCache* noptrAssetCache, const char* binPath, const char* rawPath, const char* watchPath, r2::asset::AssetType assetType) = 0;
		virtual void Shutdown() = 0;

		virtual r2::asset::AssetType GetAssetType() const = 0;
		virtual u64 GetManifestFileHandle() const = 0;
		
		virtual bool LoadManifest() = 0;
		virtual bool UnloadManifest() = 0;
		virtual const byte* GetManifestData() const = 0;
		virtual const char* FilePath() const = 0;
		
		//@NOTE(Serge): These shouldn't exist
		virtual bool AddAllFilePaths(FileList files) = 0;

#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, pln::HotReloadType hotreloadType) = 0;
		using ReloadFilePathFunc = std::function<bool(const std::vector<std::string>&, const std::string& manifestFilePath, const byte* manifestData, pln::HotReloadType hotreloadType)>;
		void SetReloadFilePathCallback(ReloadFilePathFunc func)
		{
			mReloadFilePathFunc = func;
		}

		virtual bool SaveManifest() = 0;
		virtual void Reload() = 0;
#endif
	protected:
#ifdef R2_ASSET_PIPELINE
		ReloadFilePathFunc mReloadFilePathFunc;
#endif
		AssetCache* mnoptrAssetCache = nullptr;
		AssetFile* mManifestAssetFile = nullptr;
		AssetCacheRecord mManifestCacheRecord;
		r2::asset::AssetType mAssetType;
		r2::asset::AssetHandle mManifestAssetHandle;
		char mRawPath[r2::fs::FILE_PATH_LENGTH];
		char mWatchPath[r2::fs::FILE_PATH_LENGTH];
	};
}
#endif