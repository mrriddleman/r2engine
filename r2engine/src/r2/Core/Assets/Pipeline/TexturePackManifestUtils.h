#ifndef __TEXTURE_PACKS_MANIFEST_UTILS_H__
#define __TEXTURE_PACKS_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>
#include <filesystem>
#include "r2/Render/Model/Textures/TexturePackMetaData_generated.h"

namespace r2::asset::pln::tex
{

	bool GenerateTexturePacksManifestFromJson(const std::string& jsonManifestFilePath, const std::string& outputDir);
	bool GenerateTexturePacksManifestFromDirectories(const std::string& binFilePath, const std::string& jsonFilePath, const std::string& directory, const std::string& binDir);
	bool FindTexturePacksManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);

	bool HasTexturePackInManifestFile(const std::string& manifestFilePath, const std::string& packName);
	bool HasTexturePathInManifestFile(const std::string& manifestFilePath, const std::string& packName, const std::string& filePath);
	std::filesystem::path GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir);

	struct TexturePackMetaFileSideEntry
	{
		std::string textureName;
		flat::CubemapSide cubemapSide;
	};

	struct TexturePackMetaFileMipLevel
	{
		u32 level;
		std::vector< TexturePackMetaFileSideEntry> sides;
	};

	struct TexturePackMetaFile
	{
		flat::TextureType type;
		s32 desiredMipLevels;
		flat::MipMapFilter filter;
		std::vector<TexturePackMetaFileMipLevel> mipLevels;
	};

	void MakeTexturePackMetaFileFromFlat(const flat::TexturePackMetaData* flatMeta, TexturePackMetaFile& metaFile);

	bool GenerateTexturePackMetaJSONFile(const std::filesystem::path& outputDir, const TexturePackMetaFile& metaFile);
}

#endif
#endif
