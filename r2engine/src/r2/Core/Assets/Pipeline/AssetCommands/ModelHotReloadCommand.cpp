#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/ModelHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/AssetConverterUtils.h"

namespace r2::asset::pln
{
	const std::string RMDL_EXT = ".rmdl";

	ModelHotReloadCommand::ModelHotReloadCommand()
	{
		
	}

	void ModelHotReloadCommand::Init(Milliseconds delay)
	{
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

		return { createDirCmd };
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
				int result = pln::assetconvert::RunModelConverter(inputDirPath.string(), outputDirPath.string(), materialManifestPath.string());
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
						int result = pln::assetconvert::RunModelConverter(p.path().string(), outputPath.string(), materialManifestPath.string());

						R2_CHECK(result == 0, "Failed to create the rmdl files of: %s\n", p.path().string().c_str());
					}
				}
			}
		}
	}

}

#endif