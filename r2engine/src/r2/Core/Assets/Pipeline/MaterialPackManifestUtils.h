#ifndef __MATERIAL_PACK_MANIFEST_UTILS_H__
#define __MATERIAL_PACK_MANIFEST_UTILS_H__
#ifdef R2_ASSET_PIPELINE

#include <string>
//#include "r2/Render/Model/Material.h"

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
	//bool GenerateMaterialPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	//bool FindMaterialPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);


	//@Temporary
	//bool GenerateMaterialParamsFromOldMaterialPack(const flat::Material* const materialData, const std::string& pathOfSource, const std::string& outputDir);
	
	//@Temporary
	bool GenerateMaterialFromOldMaterialParamsPack(const flat::MaterialParams* const materialParamsData, const std::string& pathOfSource, const std::string& ouptDir);

	bool RegenerateMaterialParamsPackManifest(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	bool GenerateMaterialParamsFromJSON(const std::string& outputDir, const std::string& path);
	bool GenerateMaterialParamsPackManifestFromJson(const std::string& jsonMaterialParamsPackManifestFilePath, const std::string& outputDir);
	bool GenerateMaterialParamsPackManifestFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir);
	bool FindMaterialParamsPackManifestFile(const std::string& directory, const std::string& stemName, std::string& outPath, bool isBinary);
	bool GenerateBinaryParamsPackManifest(const std::string& binaryDir, const std::string& binFilePath, const std::string& rawFilePath);

	
}

#endif
#endif