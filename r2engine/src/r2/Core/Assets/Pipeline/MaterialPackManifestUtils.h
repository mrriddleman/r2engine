#ifndef __MATERIAL_PACK_MANIFEST_UTILS_H__
#define __MATERIAL_PACK_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>
#include "r2/Render/Model/Materials/Material.h"

namespace flat
{
	struct Material;
	struct MaterialParams;
}

namespace r2::asset::pln
{
	using FindMaterialPackManifestFileFunc = std::function<bool(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary)>;
	using GenerateMaterialPackManifestFromDirectoriesFunc = std::function<bool(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)>;

	bool GenerateMaterialFromJSON(const std::string& outputDir, const std::string& path);
	bool GenerateMaterialPackManifestFromJson(const std::string& jsonMaterialPackManifestFilePath, const std::string& outputDir);
	bool GenerateBinaryMaterialPackManifest(const std::string& binaryDir, const std::string& binFilePath, const std::string& rawFilePath);
	bool GenerateMaterialPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);
	bool RegenerateMaterialPackManifest(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);

	bool SaveMaterialsToMaterialPackManifestFile(const std::vector<r2::mat::Material>& materials, const std::string& binFilePath, const std::string& rawFilePath);

	//@Temporary
	//bool GenerateMaterialFromOldMaterialParamsPack(const flat::MaterialParams* const materialParamsData, const std::string& pathOfSource, const std::string& ouptDir);
}

#endif
#endif