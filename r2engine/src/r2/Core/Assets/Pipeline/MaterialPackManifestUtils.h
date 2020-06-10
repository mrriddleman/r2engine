#ifndef __MATERIAL_PACK_MANIFEST_UTILS_H__
#define __MATERIAL_PACK_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>

namespace r2::asset::pln
{
	bool GenerateMaterialPackManifestFromJson(const std::string& materialPackManifestFilePath);
	bool GenerateMaterialPackManifestFromDirectories(const std::string& filePath, const std::string& directory);
	bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);
}

#endif
#endif