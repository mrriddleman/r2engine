#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/LevelPackHotReloadCommand.h"

#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset::pln
{
	const std::string LEVEL_PACK_MANIFEST_FBS = "LevelPack.fbs";

	LevelPackHotReloadCommand::LevelPackHotReloadCommand()
	{

	}

	void LevelPackHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateLevelPackIfNecessary();
	}

	void LevelPackHotReloadCommand::Update()
	{

	}

	bool LevelPackHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> LevelPackHotReloadCommand::DirectoriesToCreate() const
	{
		AssetHotReloadCommand::CreateDirCmd createDirCmd;

		createDirCmd.pathsToCreate = { mLevelPackBinFilePath, mLevelPackRawFilePath };
		createDirCmd.startAtParent = true;
		createDirCmd.startAtOne = false;

		return { createDirCmd };
	}

	void LevelPackHotReloadCommand::SetLevelPackBinFilePath(const std::string& levelPackFilePath)
	{
		mLevelPackBinFilePath = levelPackFilePath;
	}

	void LevelPackHotReloadCommand::SetLevelPackRawFilePath(const std::string& levelPackRawFilePath)
	{
		mLevelPackRawFilePath = levelPackRawFilePath;
	}

	void LevelPackHotReloadCommand::GenerateLevelPackIfNecessary()
	{
		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char levelPackManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), levelPackManifestSchemaPath, LEVEL_PACK_MANIFEST_FBS.c_str());

		std::filesystem::path binaryPath = mLevelPackBinFilePath;
		std::string binManifestDir = binaryPath.parent_path().string();

		std::filesystem::path rawPath = mLevelPackRawFilePath;
		std::string rawManifestDir = rawPath.parent_path().string();

		std::string levelPackManifestFile = "";

		FindManifestFile(binManifestDir, binaryPath.stem().string(), ".rlpk", levelPackManifestFile, true);

		if (levelPackManifestFile.empty())
		{
			if (FindManifestFile(rawManifestDir, rawPath.stem().string(), ".rlpk", levelPackManifestFile, false))
			{
				//Generate the binary file from json
				bool generatedBinFile = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binManifestDir, levelPackManifestSchemaPath, rawPath.string());

				R2_CHECK(generatedBinFile, "Failed to generate the binary file: %s", binaryPath.string().c_str());
			}
			else
			{
				//generate both
				//@TODO(Serge): version
				bool success = r2::asset::pln::GenerateLevelPackDataFromDirectories(1, binaryPath.string(), rawPath.string(), std::filesystem::path(binManifestDir).parent_path().string(), std::filesystem::path(rawManifestDir).parent_path().string());

				R2_CHECK(success, "Failed to generate the binary file: %s and the raw file: %s", binaryPath.string().c_str(), rawPath.string().c_str());
			}
		}
	}

}

#endif