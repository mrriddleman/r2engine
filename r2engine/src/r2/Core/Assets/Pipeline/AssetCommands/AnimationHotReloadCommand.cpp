#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/AnimationHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/AssetConverterUtils.h"

namespace r2::asset::pln
{
	const std::string RANM_EXT = ".ranm";

	AnimationHotReloadCommand::AnimationHotReloadCommand()
	{
		
	}

	void AnimationHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateRANMFilesIfNeeded();
	}

	void AnimationHotReloadCommand::Update()
	{

	}

	bool AnimationHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> AnimationHotReloadCommand::DirectoriesToCreate() const
	{
		AssetHotReloadCommand::CreateDirCmd createDirCmd;

		createDirCmd.pathsToCreate = mBinaryAnimationDirectories;
		createDirCmd.startAtParent = false;
		createDirCmd.startAtOne = false;

		return { createDirCmd };
	}

	void AnimationHotReloadCommand::AddBinaryAnimationDirectories(const std::vector<std::string>& binaryAnimationDirectories)
	{
		mBinaryAnimationDirectories.insert(mBinaryAnimationDirectories.end(), binaryAnimationDirectories.begin(), binaryAnimationDirectories.end());
	}

	void AnimationHotReloadCommand::AddRawAnimationDirectories(const std::vector<std::string>& rawAnimationDirectories)
	{
		mRawAnimationDirectories.insert(mRawAnimationDirectories.end(), rawAnimationDirectories.begin(), rawAnimationDirectories.end());
	}

	std::filesystem::path AnimationHotReloadCommand::GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir)
	{
		std::vector<std::filesystem::path> pathsSeen;

		std::filesystem::path thePath = inputPath;
		thePath.make_preferred();

		std::filesystem::path outputFilenamePath = "";

		if (inputPath.has_extension())
		{
			thePath = inputPath.parent_path();

			outputFilenamePath = inputPath.filename().replace_extension(RANM_EXT);
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

	void AnimationHotReloadCommand::GenerateRANMFilesIfNeeded()
	{
		R2_CHECK(mRawAnimationDirectories.size() == mBinaryAnimationDirectories.size(), "These should be the same");
	

		const u64 numWatchPaths = mRawAnimationDirectories.size();

		for (u64 i = 0; i < numWatchPaths; ++i)
		{
			std::filesystem::path inputDirPath = mRawAnimationDirectories[i];
			inputDirPath.make_preferred();
			std::filesystem::path outputDirPath = mBinaryAnimationDirectories[i];
			outputDirPath.make_preferred();

			if (std::filesystem::is_empty(outputDirPath))
			{
			//	int result = pln::assetconvert::RunAnimationConverter(inputDirPath.string(), outputDirPath.string());
			//	R2_CHECK(result == 0, "Failed to create the ranim files of: %s\n", inputDirPath.string().c_str());
			}
			else
			{
				//for now I guess we'll just check to see if we have an equivalent directory in the sub directories (and if they aren't empty
				//if not then run the converter for that directory, otherwise skip. Unfortunately, this will not catch the case of individual files not being converted...
				for (const auto& p : std::filesystem::directory_iterator(inputDirPath))
				{
					std::filesystem::path outputPath = GetOutputFilePath(p.path(), mRawAnimationDirectories[i], mBinaryAnimationDirectories[i]);
					outputPath.make_preferred();

					if (!std::filesystem::exists(outputPath))
					{
						std::filesystem::create_directory(outputPath);
					}

					if (std::filesystem::is_empty(outputPath))
					{
				//		int result = pln::assetconvert::RunAnimationConverter(p.path().string(), outputPath.string());

					//	R2_CHECK(result == 0, "Failed to create the ranm files of: %s\n", p.path().string().c_str());
					}
				}
			}
		}
	}

}

#endif