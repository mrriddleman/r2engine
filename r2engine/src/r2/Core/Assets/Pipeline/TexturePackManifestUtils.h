#ifndef __TEXTURE_PACKS_MANIFEST_UTILS_H__
#define __TEXTURE_PACKS_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>

namespace r2::asset::pln::tex
{
	bool GenerateTexturePacksManifestFromJson(const std::string& jsonManifestFilePath, const std::string& outputDir);
	bool GenerateTexturePacksManifestFromDirectories(const std::string& binFilePath, const std::string& jsonFilePath, const std::string& directory, const std::string& binDir);
	bool FindTexturePacksManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);
}

#endif
#endif
