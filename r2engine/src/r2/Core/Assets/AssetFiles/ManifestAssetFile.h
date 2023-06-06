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
		virtual bool Init(const char* path) = 0;
		virtual r2::asset::EngineAssetType GetAssetType() = 0;
		virtual bool AddAllFilePaths(FileList files) = 0;
		virtual bool HasFilePath(const char* filePath) = 0;
		virtual u64 GetManifestFileHandle() = 0;
	};
}
#endif