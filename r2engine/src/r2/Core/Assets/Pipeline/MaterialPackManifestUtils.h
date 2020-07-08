#ifndef __MATERIAL_PACK_MANIFEST_UTILS_H__
#define __MATERIAL_PACK_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>

namespace r2::asset::pln
{
	bool GenerateMaterialPackManifestFromJson(const std::string& jsonMaterialPackManifestFilePath, const std::string& outputDir);
	bool GenerateMaterialPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);
}

#endif
#endif