#ifndef __MANIFEST_ASSET_FILE_H__
#define __MANIFEST_ASSET_FILE_H__

#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/AssetFiles/AssetFile.h"

namespace r2::asset
{
	class ManifestAssetFile : public AssetFile
	{
	public:
		virtual ~ManifestAssetFile() {}
		virtual bool Init(const char* path, r2::asset::AssetType assetType) = 0;
		virtual r2::asset::AssetType GetAssetType() const = 0;
		virtual bool AddAllFilePaths(FileList files) = 0;
		virtual u64 GetManifestFileHandle() const = 0;
		virtual bool NeedsManifestData() const = 0;
		virtual void SetManifestData(const byte* manifestData) = 0;

#ifdef R2_ASSET_PIPELINE
		virtual bool ReloadFilePath(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, HotReloadType hotreloadType) = 0;
		using ReloadFilePathFunc = std::function<bool(const std::vector<std::string>&, const std::string& manifestFilePath, const byte* manifestData, HotReloadType hotreloadType)>;
		void SetReloadFilePathCallback(ReloadFilePathFunc func)
		{
			mReloadFilePathFunc = func;
		}
#endif
	protected:
#ifdef R2_ASSET_PIPELINE
		ReloadFilePathFunc mReloadFilePathFunc;
#endif
	};
}
#endif