#include "r2pch.h"

#if defined(R2_ASSET_PIPELINE) && defined(R2_EDITOR)
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include <filesystem>
#include "r2/Core/Assets/AssetFiles/AssetFile.h"
#include "r2/Render/Model/Materials/MaterialTypes.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Audio/SoundDefinitionHelpers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace r2::asset::pln
{
	const std::string LEVEL_DATA_FBS = "LevelData.fbs";
	const std::string LEVEL_PACK_DATA_FBS = "LevelPack.fbs";

	bool WriteNewLevelDataFromBinary(const std::string& binLevelPath, const std::string& rawJSONPath, const std::string& fbs, const void* data, u32 dataSize)
	{
		utils::WriteFile(binLevelPath, (char*)data, dataSize);

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char levelDataSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), levelDataSchemaPath, fbs.c_str());

		std::filesystem::path binaryPath = binLevelPath;

		std::filesystem::path jsonPath = rawJSONPath;

		bool generatedJSON = r2::asset::pln::flathelp::GenerateFlatbufferJSONFile(jsonPath.parent_path().string(), levelDataSchemaPath, binLevelPath);

		bool generatedBinary = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binaryPath.parent_path().string(), levelDataSchemaPath, jsonPath.string());

		return generatedJSON && generatedBinary;
	}

	std::vector<flatbuffers::Offset<flat::MaterialName>> MakeMaterialNameVectorFromMaterialNames(flatbuffers::FlatBufferBuilder& builder, const r2::SArray<r2::mat::MaterialName>* materialNames)
	{
		const u32 numMaterialNames = r2::sarr::Size(*materialNames);

		std::vector<flatbuffers::Offset<flat::MaterialName>> flatMaterialNames = {};
		flatMaterialNames.reserve(numMaterialNames);

		for (u32 i = 0; i < numMaterialNames; ++i)
		{
			r2::mat::MaterialName materialName = r2::sarr::At(*materialNames, i);

			//@TODO(Serge): UUID
			auto materialAssetName = flat::CreateAssetName(builder, 0, materialName.assetName.hashID, builder.CreateString(materialName.assetName.assetNameString));
			auto materialPackAssetName = flat::CreateAssetName(builder, 0, materialName.packName.hashID, builder.CreateString(materialName.packName.assetNameString));

			flatMaterialNames.push_back(flat::CreateMaterialName(builder, materialAssetName, materialPackAssetName));
		}
		return flatMaterialNames;
	}

	//@TODO(Serge): r2::SArray<r2::asset::AssetHandle> should be r2::SArray<flat::AssetName>
	std::vector<flatbuffers::Offset<flat::AssetName>> MakeAssetNamesFromFileList(flatbuffers::FlatBufferBuilder& builder, r2::fs::utils::Directory directory, r2::asset::AssetType assetType, const r2::SArray<r2::asset::AssetHandle>* assetHandles)
	{
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
		r2::asset::AssetLib& assetLib = MENG.GetAssetLib();
		std::vector<flatbuffers::Offset<flat::AssetName>> assetNames;

		size_t numFiles = r2::sarr::Size(*assetHandles);

		char sanitizedFilePathURI[r2::fs::FILE_PATH_LENGTH];
		char assetNameStr[r2::fs::FILE_PATH_LENGTH];

		for (size_t i = 0; i < numFiles; ++i)
		{

			const auto assetHandle = r2::sarr::At(*assetHandles, i);

			//This shouldn't really exist - should just get the AssetName and use that
			{
				const r2::asset::AssetFile* assetFile = r2::asset::lib::GetAssetFileForAsset(assetLib, r2::asset::Asset(assetHandle.handle, assetType));//gameAssetManager.GetAssetFile(assetHandle);

				std::filesystem::path filePath = assetFile->FilePath();

				r2::fs::utils::SanitizeSubPath(filePath.string().c_str(), sanitizedFilePathURI);

				r2::asset::MakeAssetNameStringForFilePath(sanitizedFilePathURI, assetNameStr, assetType);
			}

			
			//@TODO(Serge): UUID
			auto assetName = flat::CreateAssetName(builder, 0, assetHandle.handle, builder.CreateString(assetNameStr));

			assetNames.push_back(assetName);
		}

		return assetNames;
	}

	//@TODO(Serge): r2::SArray<u64> should be r2::SArray<flat::AssetName>
	std::vector<flatbuffers::Offset<flat::AssetName>> MakePackReferencesFromSoundBankAssetNames(flatbuffers::FlatBufferBuilder& builder, const r2::SArray<u64>* soundBankAssetNames)
	{
		std::vector<flatbuffers::Offset<flat::AssetName>> assetNames = {};

		size_t numSoundBanks = r2::sarr::Size(*soundBankAssetNames);

		for (size_t i = 0; i < numSoundBanks; ++i)
		{
			u64 assetBankName = r2::sarr::At(*soundBankAssetNames, i);
			const char* soundBankName = r2::audio::GetSoundBankNameFromAssetName(assetBankName);

			char sanitizedFilePathURI[r2::fs::FILE_PATH_LENGTH];

			r2::fs::utils::SanitizeSubPath(soundBankName, sanitizedFilePathURI);

			auto assetName = flat::CreateAssetName(builder, 0, assetBankName, builder.CreateString(sanitizedFilePathURI));

			assetNames.push_back(assetName);
		}

		return assetNames;
	}

	bool SaveLevelData(
		const r2::ecs::ECSCoordinator* coordinator,
		const std::string& binLevelPath,
		const std::string& rawJSONPath,
		const r2::Level &editorLevel)
	{
		std::filesystem::path fsBinLevelPath = binLevelPath;
		std::filesystem::path fsRawLevelPath = rawJSONPath;

		fsBinLevelPath.replace_extension(".rlvl");
		std::string groupName = fsBinLevelPath.parent_path().stem().string();
		std::filesystem::path groupNamePath = groupName;
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

		std::vector<flatbuffers::Offset<flat::AssetName>> modelPackReferences = MakeAssetNamesFromFileList(builder, r2::fs::utils::Directory::MODELS, r2::asset::RMODEL, editorLevel.GetModelAssets());
		std::vector<flatbuffers::Offset<flat::MaterialName>> materialNames = MakeMaterialNameVectorFromMaterialNames(builder, editorLevel.GetMaterials());
		std::vector<flatbuffers::Offset<flat::AssetName>> soundReferences = MakePackReferencesFromSoundBankAssetNames(builder, editorLevel.GetSoundBankAssetNames());
		
		coordinator->SerializeECS(builder, entityVec, componentDataArray);

		auto levelAssetHash= r2::asset::GetAssetNameForFilePath(fsBinLevelPath.string().c_str(), r2::asset::LEVEL);
		auto groupAssetHash = r2::asset::GetAssetNameForFilePath(groupName.c_str(), r2::asset::LEVEL_GROUP);

		auto levelAssetName = flat::CreateAssetName(builder, 0, levelAssetHash, builder.CreateString(levelName));
		auto levelAssetRef = flat::CreateAssetRef(builder, levelAssetName, builder.CreateString(binLevelPath), builder.CreateString(rawJSONPath));

		auto groupAssetName = flat::CreateAssetName(builder, 0, groupAssetHash, builder.CreateString(groupName.c_str()));

		auto levelData = flat::CreateLevelData(
			builder,
			editorLevel.GetVersion(), levelAssetRef, groupAssetName,
			numEntities,
			builder.CreateVector(entityVec),
			builder.CreateVector(componentDataArray),
			builder.CreateVector(materialNames),
			builder.CreateVector(modelPackReferences),
			builder.CreateVector(soundReferences)
			);

		builder.Finish(levelData);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		return WriteNewLevelDataFromBinary(binLevelPath, rawJSONPath, LEVEL_DATA_FBS, buf, size);
	}

	bool GenerateLevelPackDataFromDirectories(u32 version, const std::string& binFilePath, const std::string& rawFilePath, const std::string& binaryDir, const std::string& rawDir)
	{
		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<flat::LevelGroupData>> flatLevelGroups;

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;
		char levelDataSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), levelDataSchemaPath, LEVEL_PACK_DATA_FBS.c_str());

		for (auto& groupDir : std::filesystem::directory_iterator(rawDir))
		{
			if (!std::filesystem::is_directory(groupDir) ||
				std::filesystem::file_size(groupDir.path()) <= 0 ||
				groupDir.path().filename() == ".DS_Store")
			{
				continue;
			}

			std::vector<flatbuffers::Offset<flat::AssetRef>> levelsInGroup;

			char sanitizedFilePathURI[r2::fs::FILE_PATH_LENGTH];
			char levelName[r2::fs::FILE_PATH_LENGTH];

			for (auto& file : std::filesystem::directory_iterator(groupDir))
			{
				if (std::filesystem::file_size(file.path()) <= 0 ||
					file.path().filename() == ".DS_Store" ||
					file.path().extension() != ".json")
				{
					continue;
				}

				std::filesystem::path levelPath = file;
				levelPath.replace_extension(".rlvl");

				r2::fs::utils::SanitizeSubPath(levelPath.string().c_str(), sanitizedFilePathURI);

				r2::asset::MakeAssetNameStringForFilePath(sanitizedFilePathURI, levelName, r2::asset::LEVEL);

				auto assetName = flat::CreateAssetName(builder, 0, r2::asset::GetAssetNameForFilePath(sanitizedFilePathURI, r2::asset::LEVEL), builder.CreateString(levelName));

				std::filesystem::path binLevelPath = binaryDir;
				binLevelPath /= levelName;

				r2::fs::utils::SanitizeSubPath(binLevelPath.string().c_str(), sanitizedFilePathURI);

				r2::fs::utils::SanitizeSubPath(file.path().string().c_str(), levelName);

				auto assetRef = flat::CreateAssetRef(builder, assetName, builder.CreateString(sanitizedFilePathURI), builder.CreateString(levelName));

				levelsInGroup.push_back(assetRef);
			}

			std::filesystem::path groupPath = groupDir.path();

			auto assetName = flat::CreateAssetName(builder, 0, r2::asset::Asset::GetAssetNameForFilePath(groupPath.string().c_str(), r2::asset::LEVEL_GROUP), builder.CreateString(groupPath.stem().string()));

			flat::CreateLevelGroupData(builder, version, assetName, builder.CreateVector(levelsInGroup));
		}

		std::filesystem::path levelPackPath = binFilePath;

		auto assetName = flat::CreateAssetName(builder, 0, r2::asset::Asset::GetAssetNameForFilePath(binFilePath.c_str(), r2::asset::LEVEL_PACK), builder.CreateString(levelPackPath.filename().string()));

		auto levelPackData = flat::CreateLevelPackData(builder, version, assetName, builder.CreateVector(flatLevelGroups));

		builder.Finish(levelPackData);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		return WriteNewLevelDataFromBinary(binFilePath, rawFilePath, LEVEL_PACK_DATA_FBS, buf, size);
	}

	bool GenerateEmptyLevelPackFile(const std::string& binFilePath, const std::string& rawFilePath)
	{
		R2_CHECK(false, "Not implemented");
		return false;
	}

	bool SaveLevelPackData(u32 version, const std::vector<LevelGroup>& levelGroups, const std::string& binFilePath, const std::string& rawFilePath)
	{
		flatbuffers::FlatBufferBuilder builder;

		std::vector<flatbuffers::Offset<flat::LevelGroupData>> flatLevelGroups;
		
		char levelName[r2::fs::FILE_PATH_LENGTH];

		for (size_t i = 0; i < levelGroups.size(); ++i)
		{
			std::vector<flatbuffers::Offset<flat::AssetRef>> levelAssetRefs;

			for (size_t j = 0; j < levelGroups[i].levelReferences.size(); ++j)
			{
				r2::asset::MakeAssetNameStringForFilePath(levelGroups[i].levelReferences[j].binPath.string().c_str(), levelName, r2::asset::LEVEL);

				auto assetName = flat::CreateAssetName(builder, 0, r2::asset::GetAssetNameForFilePath(levelGroups[i].levelReferences[j].binPath.string().c_str(), r2::asset::LEVEL), builder.CreateString(levelName));

				auto assetRef = flat::CreateAssetRef(builder, assetName, builder.CreateString(levelGroups[i].levelReferences[j].binPath.string()), builder.CreateString(levelGroups[i].levelReferences[j].rawPath.string()));

				levelAssetRefs.push_back(assetRef);
			}

			auto assetName = flat::CreateAssetName(builder, 0, r2::asset::GetAssetNameForFilePath(levelGroups[i].groupName.c_str(), r2::asset::LEVEL_GROUP), builder.CreateString(levelGroups[i].groupName));

			flatLevelGroups.push_back( flat::CreateLevelGroupData(builder, version, assetName, builder.CreateVector(levelAssetRefs)) );

		}

		std::filesystem::path levelPackPath = binFilePath;

		auto assetName = flat::CreateAssetName(builder, 0, r2::asset::Asset::GetAssetNameForFilePath(binFilePath.c_str(), r2::asset::LEVEL_PACK), builder.CreateString(levelPackPath.filename().string()));

		auto levelPackData = flat::CreateLevelPackData(builder, version, assetName, builder.CreateVector(flatLevelGroups));

		builder.Finish(levelPackData);

		byte* buf = builder.GetBufferPointer();
		u32 size = builder.GetSize();

		return WriteNewLevelDataFromBinary(binFilePath, rawFilePath, LEVEL_PACK_DATA_FBS, buf, size);
	}

}

#endif