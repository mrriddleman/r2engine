#ifndef __TEXTURE_PACKS_MANIFEST_UTILS_H__
#define __TEXTURE_PACKS_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>
#include <filesystem>

namespace r2::asset::pln::tex
{
	bool GenerateTexturePacksManifestFromJson(const std::string& jsonManifestFilePath, const std::string& outputDir);
	bool GenerateTexturePacksManifestFromDirectories(const std::string& binFilePath, const std::string& jsonFilePath, const std::string& directory, const std::string& binDir);
	bool FindTexturePacksManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);

	bool HasTexturePackInManifestFile(const std::string& manifestFilePath, const std::string& packName);
	bool HasTexturePathInManifestFile(const std::string& manifestFilePath, const std::string& packName, const std::string& filePath);
	std::vector<std::vector<std::string>> GetAllTexturesInTexturePack(const std::string& manifestFilePath, const std::string& packName);

	std::filesystem::path GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir);
}

#endif
#endif
