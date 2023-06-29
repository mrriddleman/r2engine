#include "r2pch.h"

#if defined(R2_ASSET_PIPELINE) && defined(R2_EDITOR)
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/Level/LevelData_generated.h"
#include <filesystem>
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Render/Model/Materials/MaterialTypes.h"

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

	std::vector<flatbuffers::Offset<flat::MaterialName>> MakeMaterialNameVectorFromMaterialNames(flatbuffers::FlatBufferBuilder& builder, const std::vector<r2::mat::MaterialName>& materialNames)
	{
		std::vector<flatbuffers::Offset<flat::MaterialName>> flatMaterialNames = {};
		flatMaterialNames.reserve(materialNames.size());

		for (const r2::mat::MaterialName& materialName : materialNames)
		{
			flatMaterialNames.push_back(flat::CreateMaterialName(builder, materialName.name, materialName.packName));
		}
		return flatMaterialNames;
	}

	std::vector<flatbuffers::Offset<flat::PackReference>> MakePackReferencesFromFileList(flatbuffers::FlatBufferBuilder& builder, r2::fs::utils::Directory directory, r2::asset::AssetType assetType, const std::vector<std::string>& fileList)
	{
		std::vector<flatbuffers::Offset<flat::PackReference>> packReferences;

		size_t numFiles = fileList.size();

		char directoryPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::BuildPathFromCategory(directory, "", directoryPath);

		for (size_t i = 0; i < numFiles; ++i)
		{
			std::filesystem::path filePath = fileList[i];

			std::filesystem::path filePathURI = filePath.lexically_relative(directoryPath);

			//make sure to sanitize the filePathURI first
			char sanitizedFilePathURI[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::SanitizeSubPath(filePathURI.string().c_str(), sanitizedFilePathURI);


			const auto assetHandle = r2::asset::Asset::GetAssetNameForFilePath(sanitizedFilePathURI, assetType);

			//@TODO(Serge): if we have packs with multiple assets in them, this might be a problem
			auto packReference = flat::CreatePackReference(builder, assetHandle, builder.CreateString(sanitizedFilePathURI));

			packReferences.push_back(packReference);
		}

		return packReferences;
	}

	bool SaveLevelData(
		const r2::ecs::ECSCoordinator* coordinator,
		const std::string& binLevelPath,
		const std::string& rawJSONPath,
		const EditorLevel& editorLevel)
	{
		std::filesystem::path fsBinLevelPath = binLevelPath;
		std::filesystem::path fsRawLevelPath = rawJSONPath;

		fsBinLevelPath.replace_extension(".rlvl");
		std::string groupName = fsBinLevelPath.parent_path().stem().string();
		std::filesystem::path groupNamePath = groupName;
		std::string levelName = (groupNamePath / fsBinLevelPath.stem()).string();

		char sanitizedLevelPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(levelName.c_str(), sanitizedLevelPath);

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

		std::vector<flatbuffers::Offset<flat::PackReference>> modelPackReferences = MakePackReferencesFromFileList(builder, r2::fs::utils::Directory::MODELS, r2::asset::RMODEL, editorLevel.GetModelAssetPaths());
		std::vector<flatbuffers::Offset<flat::PackReference>> animationPackReferences = MakePackReferencesFromFileList(builder, r2::fs::utils::Directory::ANIMATIONS, r2::asset::RANIMATION, editorLevel.GetAnimationAssetPaths());
		std::vector<flatbuffers::Offset<flat::MaterialName>> materialNames = MakeMaterialNameVectorFromMaterialNames(builder, editorLevel.GetMaterialNames());
		
		//@TODO(Serge): fix this - the directory is incorrect if we have music files
		std::vector<flatbuffers::Offset<flat::PackReference>> soundReferences = MakePackReferencesFromFileList(builder, r2::fs::utils::Directory::SOUND_FX, r2::asset::SOUND, editorLevel.GetSoundPaths());
		
		coordinator->SerializeECS(builder, entityVec, componentDataArray);

		auto levelData = flat::CreateLevelData(
			builder,
			editorLevel.GetVersion(),
			r2::asset::GetAssetNameForFilePath(fsBinLevelPath.string().c_str(), r2::asset::LEVEL),
			builder.CreateString(sanitizedLevelPath),
			r2::asset::GetAssetNameForFilePath(groupName.c_str(), r2::asset::LEVEL_GROUP),
			builder.CreateString(groupName.c_str()),
			builder.CreateString(binLevelPath), 
			numEntities,
			builder.CreateVector(entityVec),
			builder.CreateVector(componentDataArray),
			builder.CreateVector(modelPackReferences),
			builder.CreateVector(animationPackReferences),
			builder.CreateVector(materialNames),
			builder.CreateVector(soundReferences));

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