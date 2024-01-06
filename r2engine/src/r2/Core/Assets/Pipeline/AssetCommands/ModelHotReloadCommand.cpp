#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/ModelHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/AssetConverterUtils.h"
#include "r2/Core/Assets/Pipeline/AssetPipelineUtils.h"
#include "r2/Core/Assets/Pipeline/FlatbufferHelpers.h"
#include "r2/Core/Assets/Pipeline/RModelsManifestUtils.h"
#include "r2/Core/File/PathUtils.h"

namespace r2::asset::pln
{
	const std::string RMDL_EXT = ".rmdl";
	const std::string RMODEL_MANIFEST_FBS = "RModelManifest.fbs";

	ModelHotReloadCommand::ModelHotReloadCommand()
	{
		
	}

	void ModelHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateModelsManifestsIfNeeded();
		GenerateRMDLFilesIfNeeded();
	}

	void ModelHotReloadCommand::Update()
	{

	}

	bool ModelHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> ModelHotReloadCommand::DirectoriesToCreate() const
	{
		AssetHotReloadCommand::CreateDirCmd createDirCmd;

		createDirCmd.pathsToCreate = mBinaryModelDirectories;
		createDirCmd.startAtParent = false;
		createDirCmd.startAtOne = false;

		AssetHotReloadCommand::CreateDirCmd createManifestDirCmd;
		
		createManifestDirCmd.pathsToCreate.insert(createManifestDirCmd.pathsToCreate.end(), mRawModelManifestPaths.begin(), mRawModelManifestPaths.end());
		createManifestDirCmd.pathsToCreate.insert(createManifestDirCmd.pathsToCreate.end(), mBinaryModelManifestPaths.begin(), mBinaryModelManifestPaths.end());

		createManifestDirCmd.startAtParent = true;
		createManifestDirCmd.startAtOne = false;

		return { createDirCmd, createManifestDirCmd };
	}

	void ModelHotReloadCommand::AddBinaryModelDirectories(const std::vector<std::string>& binaryModelDirectories)
	{
		mBinaryModelDirectories.insert(mBinaryModelDirectories.end(), binaryModelDirectories.begin(), binaryModelDirectories.end());
	}

	void ModelHotReloadCommand::AddRawModelDirectories(const std::vector<std::string>& rawModelDirectories)
	{
		mRawModelDirectories.insert(mRawModelDirectories.end(), rawModelDirectories.begin(), rawModelDirectories.end());
	}

	void ModelHotReloadCommand::AddMaterialManifestPaths(const std::vector<std::string>& materialManifestPaths)
	{
		mMaterialManifestPaths.insert(mMaterialManifestPaths.end(), materialManifestPaths.begin(), materialManifestPaths.end());
	}

	void ModelHotReloadCommand::AddBinaryModelManifestPaths(const std::vector<std::string>& binaryManifestPaths)
	{
		mBinaryModelManifestPaths.insert(mBinaryModelManifestPaths.end(), binaryManifestPaths.begin(), binaryManifestPaths.end());
	}

	void ModelHotReloadCommand::AddRawModelManifestPaths(const std::vector<std::string>& rawManifestPaths)
	{
		mRawModelManifestPaths.insert(mRawModelManifestPaths.end(), rawManifestPaths.begin(), rawManifestPaths.end());
	}

	void ModelHotReloadCommand::HandleAssetBuildRequest(const AssetBuildRequest& request)
	{
		//first thing we need to do is figure out where it belongs
		const u64 numWatchPaths = mRawModelDirectories.size();

		R2_CHECK(request.paths.size() == 1, "For now just one");

		std::filesystem::path modelPathToBuild = request.paths[0];
		std::filesystem::path inputDirPath;
		std::filesystem::path binaryModelDirectory;
		std::filesystem::path materialManifestPath;

		bool found = false;

		for (u64 i = 0; i < numWatchPaths; ++i)
		{
			inputDirPath = mRawModelDirectories[i];
			inputDirPath.make_preferred();

			binaryModelDirectory = mBinaryModelDirectories[i];
			binaryModelDirectory.make_preferred();

			materialManifestPath = mMaterialManifestPaths[i];
			materialManifestPath.make_preferred();

			if (r2::fs::utils::HasParentInPath(modelPathToBuild, inputDirPath))
			{
				found = true;
				break;
			}
		}

		
		if (found)
		{
			//now we need to figure out the proper output dir
			std::filesystem::path lexicallyRelPath = modelPathToBuild.lexically_relative(inputDirPath).parent_path();

			int result = pln::assetconvert::RunModelConverter(
				modelPathToBuild.string(),
				(binaryModelDirectory / lexicallyRelPath).string(),
				materialManifestPath.string(), mMaterialRawDirectoryPath, mEngineTexturePacksManifestPath, mAppTexturePacksManifestPath, request.forceRebuild);

			R2_CHECK(result == 0, "Failed to create the rmdl files of: %s\n", modelPathToBuild.string().c_str());


		}

	}

	std::filesystem::path ModelHotReloadCommand::GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir)
	{
		std::vector<std::filesystem::path> pathsSeen;

		std::filesystem::path thePath = inputPath;
		thePath.make_preferred();

		std::filesystem::path outputFilenamePath = "";

		if (inputPath.has_extension())
		{
			thePath = inputPath.parent_path();

			outputFilenamePath = inputPath.filename().replace_extension(RMDL_EXT);
		}

		bool found = false;

		while (!found && !thePath.empty() && thePath != thePath.root_path())
		{
			if (inputPathRootDir == thePath)
			{
				found = true;
				break;
			}

			pathsSeen.push_back(thePath.stem());
			thePath = thePath.parent_path();
		}

		if (!found)
		{
			R2_CHECK(false, "We couldn't get the output path!");
			return "";
		}

		std::filesystem::path output = outputDir;

		auto rIter = pathsSeen.rbegin();

		for (; rIter != pathsSeen.rend(); ++rIter)
		{
			output = output / *rIter;
		}

		if (!outputFilenamePath.empty())
		{
			output = output / outputFilenamePath;
		}

		return output;
	}


	void ModelHotReloadCommand::GenerateModelsManifestsIfNeeded()
	{
		R2_CHECK(mBinaryModelManifestPaths.size() == mRawModelManifestPaths.size(), "these should be the same size!");

		std::string flatbufferSchemaPath = R2_ENGINE_FLAT_BUFFER_SCHEMA_PATH;

		char rmodelManifestSchemaPath[r2::fs::FILE_PATH_LENGTH];

		r2::fs::utils::AppendSubPath(flatbufferSchemaPath.c_str(), rmodelManifestSchemaPath, RMODEL_MANIFEST_FBS.c_str());

		for (size_t i = 0; i < mBinaryModelManifestPaths.size(); ++i)
		{
			std::filesystem::path binaryPath = mBinaryModelManifestPaths[i];
			std::string binManifestDir = binaryPath.parent_path().string();

			std::filesystem::path rawPath = mRawModelManifestPaths[i];
			std::string rawManifestDir = rawPath.parent_path().string();

			std::string modelManifestFile = "";

			FindManifestFile(binManifestDir, binaryPath.stem().string(), ".rmmn", modelManifestFile, true);

			if (modelManifestFile.empty())
			{
				if (FindManifestFile(rawManifestDir, rawPath.stem().string(), ".rmmn", modelManifestFile, false))
				{
					//Generate the binary file from json
					bool generatedBinFile = r2::asset::pln::flathelp::GenerateFlatbufferBinaryFile(binManifestDir, rmodelManifestSchemaPath, rawPath.string());

					R2_CHECK(generatedBinFile, "Failed to generate the binary file: %s", binaryPath.string().c_str());
				}
				else
				{
					//generate both
					bool success = r2::asset::pln::GenerateEmptyModelManifest(1, binaryPath.string(), rawPath.string(), binManifestDir, rawManifestDir);

					R2_CHECK(success, "Failed to generate the binary file: %s and the raw file: %s", binaryPath.string().c_str(), rawPath.string().c_str());
				}
			}
		}
	}

	void ModelHotReloadCommand::GenerateRMDLFilesIfNeeded()
	{
		R2_CHECK(mRawModelDirectories.size() == mBinaryModelDirectories.size(), "These should be the same");
		R2_CHECK(mMaterialManifestPaths.size() == mBinaryModelDirectories.size(), "These should be the same");

		const u64 numWatchPaths = mRawModelDirectories.size();

		for (u64 i = 0; i < numWatchPaths; ++i)
		{
			std::filesystem::path inputDirPath = mRawModelDirectories[i];
			inputDirPath.make_preferred();
			std::filesystem::path outputDirPath = mBinaryModelDirectories[i];
			outputDirPath.make_preferred();

			std::filesystem::path materialManifestPath = mMaterialManifestPaths[i];
			materialManifestPath.make_preferred();

			if (std::filesystem::is_empty(outputDirPath))
			{
				int result = pln::assetconvert::RunModelConverter(
					inputDirPath.string(),
					outputDirPath.string(),
					materialManifestPath.string(), mMaterialRawDirectoryPath, mEngineTexturePacksManifestPath, mAppTexturePacksManifestPath, false);
				R2_CHECK(result == 0, "Failed to create the rmdl files of: %s\n", inputDirPath.string().c_str());
			}
			else
			{
				//for now I guess we'll just check to see if we have an equivalent directory in the sub directories (and if they aren't empty
				//if not then run the converter for that directory, otherwise skip. Unfortunately, this will not catch the case of individual files not being converted...
				for (const auto& p : std::filesystem::directory_iterator(inputDirPath))
				{
					std::filesystem::path outputPath = GetOutputFilePath(p.path(), mRawModelDirectories[i], mBinaryModelDirectories[i]);
					outputPath.make_preferred();

					if (!std::filesystem::exists(outputPath))
					{
						std::filesystem::create_directory(outputPath);
					}

					if (std::filesystem::is_empty(outputPath))
					{
						int result = pln::assetconvert::RunModelConverter(
							p.path().string(),
							outputPath.string(),
							materialManifestPath.string(), mMaterialRawDirectoryPath, mEngineTexturePacksManifestPath, mAppTexturePacksManifestPath, false);

						R2_CHECK(result == 0, "Failed to create the rmdl files of: %s\n", p.path().string().c_str());
					}
				}
			}
		}
	}

	void ModelHotReloadCommand::AddTextureManifestPaths(const std::string& engineTexturePacksManifestPath, const std::string& appTexturePacksManifestPath)
	{
		mEngineTexturePacksManifestPath = engineTexturePacksManifestPath;
		mAppTexturePacksManifestPath = appTexturePacksManifestPath;

	}

	void ModelHotReloadCommand::AddMaterialRawDirectory(const std::string& materialRawDirectory)
	{
		mMaterialRawDirectoryPath = materialRawDirectory;
	}

}

#endif