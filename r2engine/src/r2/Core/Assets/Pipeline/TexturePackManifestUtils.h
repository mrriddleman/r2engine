#ifndef __TEXTURE_PACKS_MANIFEST_UTILS_H__
#define __TEXTURE_PACKS_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>

namespace r2::asset::pln::tex
{
	bool GenerateTexturePacksManifestFromJson(const std::string& texturePacksManifestFilePath);
	bool GenerateTexturePacksManifestFromDirectories(const std::string& filePath, const std::string& directory);
	bool FindTexturePacksManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);
}

#endif
#endif
