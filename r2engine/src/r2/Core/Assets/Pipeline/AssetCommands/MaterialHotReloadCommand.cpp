#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/MaterialHotReloadCommand.h"

namespace r2::asset::pln
{

	MaterialHotReloadCommand::MaterialHotReloadCommand()
	{

	}

	void MaterialHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateMaterialPackManifestsIfNeeded();
	}

	void MaterialHotReloadCommand::Update()
	{

	}

	void MaterialHotReloadCommand::Shutdown()
	{

	}

	bool MaterialHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> MaterialHotReloadCommand::DirectoriesToCreate() const
	{
		CreateDirCmd createBinaryManifestDirCmd;
		createBinaryManifestDirCmd.pathsToCreate = mManifestBinaryFilePaths;
		createBinaryManifestDirCmd.startAtOne = true;
		createBinaryManifestDirCmd.startAtParent = true;

		CreateDirCmd createWatchDirBinCmd;
		createWatchDirBinCmd.pathsToCreate = mMaterialPacksWatchDirectoriesBin;
		createWatchDirBinCmd.startAtOne = true;
		createWatchDirBinCmd.startAtParent = false;

		return {createBinaryManifestDirCmd, createWatchDirBinCmd};
	}

	void MaterialHotReloadCommand::AddManifestRawFilePaths(const std::vector<std::string>& manifestRawFilePaths)
	{
		mManifestRawFilePaths.insert(mManifestRawFilePaths.end(), manifestRawFilePaths.begin(), manifestRawFilePaths.end());
	}

	void MaterialHotReloadCommand::AddManifestBinaryFilePaths(const std::vector<std::string>& manifestBinaryFilePaths)
	{
		mManifestBinaryFilePaths.insert(mManifestBinaryFilePaths.end(), manifestBinaryFilePaths.begin(), manifestBinaryFilePaths.end());
	}

	void MaterialHotReloadCommand::AddRawMaterialPacksWatchDirectories(const std::vector<std::string>& rawWatchPaths)
	{
		mMaterialPacksWatchDirectoriesRaw.insert(mMaterialPacksWatchDirectoriesRaw.end(), rawWatchPaths.begin(), rawWatchPaths.end());
	}

	void MaterialHotReloadCommand::AddBinaryMaterialPacksWatchDirectories(const std::vector<std::string>& binWatchPaths)
	{
		mMaterialPacksWatchDirectoriesBin.insert(mMaterialPacksWatchDirectoriesBin.end(), binWatchPaths.begin(), binWatchPaths.end());
	}

	void MaterialHotReloadCommand::AddFindMaterialPackManifestFunctions(const std::vector<FindMaterialPackManifestFileFunc>& findFuncs)
	{
		for (auto& func : findFuncs)
		{
			mFindMaterialFuncs.push_back(func);
		}
	}

	void MaterialHotReloadCommand::AddGenerateMaterialPackManifestsFromDirectoriesFunctions(std::vector<GenerateMaterialPackManifestFromDirectoriesFunc> genFuncs)
	{
		for (auto& func : genFuncs)
		{
			mGenerateMaterialPackFuncs.push_back(func);
		}
	}

	void MaterialHotReloadCommand::GenerateMaterialPackManifestsIfNeeded()
	{
		R2_CHECK(mManifestBinaryFilePaths.size() == mManifestRawFilePaths.size(),
			"these should be the same size!");

		R2_CHECK(mFindMaterialFuncs.size() == mManifestBinaryFilePaths.size(), "these should be the same size");
		R2_CHECK(mGenerateMaterialPackFuncs.size() == mManifestBinaryFilePaths.size(), "these should be the same size");

		for (size_t i = 0; i < mManifestBinaryFilePaths.size(); ++i)
		{
			std::string manifestPathBin = mManifestBinaryFilePaths[i];
			std::string manifestPathRaw = mManifestRawFilePaths[i];
			std::string materialPackDirRaw = mMaterialPacksWatchDirectoriesRaw[i];
			std::string materialPackDirBin = mMaterialPacksWatchDirectoriesBin[i];

			std::filesystem::path binaryPath = manifestPathBin;
			std::string binManifestDir = binaryPath.parent_path().string();

			std::filesystem::path rawPath = manifestPathRaw;
			std::string rawManifestDir = rawPath.parent_path().string();

			std::string materialPackManifestFile;

			FindMaterialPackManifestFileFunc FindFunc = mFindMaterialFuncs[i];
			GenerateMaterialPackManifestFromDirectoriesFunc GenerateFromDirFunc = mGenerateMaterialPackFuncs[i];

			FindFunc(binManifestDir, binaryPath.stem().string(), materialPackManifestFile, true);

			if (materialPackManifestFile.empty())
			{
				if (FindFunc(rawManifestDir, rawPath.stem().string(), materialPackManifestFile, false))
				{
					if (!std::filesystem::remove(materialPackManifestFile))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from json!");
						return;
					}
				}

				if (!GenerateFromDirFunc(binaryPath.string(), rawPath.string(), materialPackDirBin, materialPackDirRaw))
				{
					R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
					return;
				}

			}
		}
	}


}

#endif