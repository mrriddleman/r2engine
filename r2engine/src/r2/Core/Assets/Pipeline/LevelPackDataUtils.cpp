#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/File/PathUtils.h"
#include <filesystem>

namespace r2::asset::pln
{
	const std::string LEVEL_DATA_FBS = "LevelData.fbs";

	bool WriteNewLevelDataFromBinary(const std::string& binLevelPath, const std::string& rawJSONPath, const void* data, u32 dataSize)
	{
		utils::WriteFile(binLevelPath, (char*)data, dataSize);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char levelDataSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), levelDataSchemaPath, LEVEL_DATA_FBS.c_str());

		std::filesystem::path binaryPath = binLevelPath;

		std::filesystem::path jsonPath = rawJSONPath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), levelDataSchemaPath, binLevelPath);

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binaryPath.parent_path().string(), levelDataSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;
	}

	void RegenerateLevelDataFromDirectories(const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{
		R2_CHECK(false, "not implemented");
	}

	bool GenerateEmptyLevelPackFile(const std::string& binFilePath, const std::string& rawFilePath)
	{
		R2_CHECK(false, "Not implemented");
		return false;
	}

}

#endif