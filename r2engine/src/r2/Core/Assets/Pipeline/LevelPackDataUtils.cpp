#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/Level/LevelData_generated.h"
#include <filesystem>
#include "r2/Core/Assets/AssetFiles/AssetFile.h"

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

	std::vector<flatbuffers::Offset<flat::PackReference>> MakePackReferencesFromFileList(flatbuffers::FlatBufferBuilder& builder, r2::fs::utils::Directory directory, const std::vector<r2::asset::AssetFile*>& fileList)
	{
		std::vector<flatbuffers::Offset<flat::PackReference>> packReferences;

		size_t numFiles = fileList.size();

		char directoryPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::BuildPathFromCategory(directory, "", directoryPath);

		for (size_t i = 0; i < numFiles; ++i)
		{
			AssetFile* assetFile = fileList[i];

			R2_CHECK(assetFile != nullptr, "asset file is null?");

			std::filesystem::path filePath = assetFile->FilePath();

			std::filesystem::path filePathURI = filePath.lexically_relative(directoryPath);

			//make sure to sanitize the filePathURI first
			char sanitizedFilePathURI[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::SanitizeSubPath(filePathURI.string().c_str(), sanitizedFilePathURI);

			//@TODO(Serge): if we have packs with multiple assets in them, this might be a problem
			auto packReference = flat::CreatePackReference(builder, assetFile->GetAssetHandle(0), builder.CreateString(sanitizedFilePathURI));

			packReferences.push_back(packReference);
		}

		return packReferences;
	}

	bool SaveLevelData(
		const r2::ecs::ECSCoordinator* coordinator,
		u32 version,
		const std::string& binLevelPath,
		const std::string& rawJSONPath,
		const std::vector<AssetFile*>& modelFiles,
		const std::vector<AssetFile*>& animationFiles)
	{
		std::filesystem::path fsBinLevelPath = binLevelPath;
		std::filesystem::path fsRawLevelPath = rawJSONPath;

		std::string groupName = fsBinLevelPath.parent_path().stem().string();
		std::string levelName = fsBinLevelPath.stem().string();

		if (!std::filesystem::exists(fsBinLevelPath.parent_path()))
		{
			std::error_code err;
			std::filesystem::create_directories(fsBinLevelPath.parent_path(),err);
			printf("Error creating directories for path: %s, with error: %s\n", fsBinLevelPath.parent_path().string().c_str(), err.message().c_str());
		}

		if (!std::filesystem::exists(fsRawLevelPath.parent_path()))
		{
			std::error_code err;
			std::filesystem::create_directories(fsRawLevelPath.parent_path(), err);
			printf("Error creating directories for path: %s, with error: %s\n", fsRawLevelPath.parent_path().string().c_str(), err.message().c_str());
		}

		flatbuffers::FlatBufferBuilder builder;

		u32 numEntities = coordinator->NumLivingEntities();

		std::vector<flatbuffers::Offset<flat::EntityData>> entityVec;
		std::vector<flatbuffers::Offset<flat::ComponentArrayData>> componentDataArray;

		std::vector<flatbuffers::Offset<flat::PackReference>> modelPackReferences = MakePackReferencesFromFileList(builder, r2::fs::utils::Directory::MODELS, modelFiles);
		std::vector<flatbuffers::Offset<flat::PackReference>> animationPackReferences = MakePackReferencesFromFileList(builder, r2::fs::utils::Directory::ANIMATIONS, animationFiles);

		coordinator->SerializeECS(builder, entityVec, componentDataArray);

		auto levelData = flat::CreateLevelData(
			builder,
			version,
			STRING_ID(levelName.c_str()),
			builder.CreateString(levelName),
			STRING_ID(groupName.c_str()),
			builder.CreateString(groupName.c_str()),
			builder.CreateString(binLevelPath), 
			numEntities,
			builder.CreateVector(entityVec),
			builder.CreateVector(componentDataArray),
			builder.CreateVector(modelPackReferences),
			builder.CreateVector(animationPackReferences));

		builder.Finish(levelData);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		return WriteNewLevelDataFromBinary(binLevelPath, rawJSONPath, buf, size);
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