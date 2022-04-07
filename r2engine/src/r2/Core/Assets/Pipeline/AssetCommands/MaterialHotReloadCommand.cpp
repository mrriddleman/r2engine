#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/MaterialHotReloadCommand.h"
#include "r2/Render/Model/Material.h"

namespace r2::asset::pln
{

	MaterialHotReloadCommand::MaterialHotReloadCommand()
	{

	}

	void MaterialHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateMaterialPackManifestsIfNeeded();

		for (const auto& path : mMaterialPacksWatchDirectoriesRaw)
		{
			FileWatcher fw;
			fw.Init(delay, path);

			fw.AddModifyListener(std::bind(&MaterialHotReloadCommand::MaterialChangedRequest, this, std::placeholders::_1));
			fw.AddCreatedListener(std::bind(&MaterialHotReloadCommand::MaterialAddedRequest, this, std::placeholders::_1));
			fw.AddRemovedListener(std::bind(&MaterialHotReloadCommand::MaterialRemovedRequest, this, std::placeholders::_1));

			mFileWatchers.push_back(fw);
		}
	}

	void MaterialHotReloadCommand::Update()
	{
		for (auto& fw : mFileWatchers)
		{
			fw.Run();
		}
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

	void MaterialHotReloadCommand::MaterialChangedRequest(const std::string& changedPath)
	{
		std::filesystem::path changedMaterialPath = changedPath;

		R2_CHECK(changedMaterialPath.extension() == ".json", "This should be a json file!");

		size_t index = 0;
		bool found = false;
		for (const std::string& rawWatchDir : mMaterialPacksWatchDirectoriesRaw)
		{
			std::filesystem::path rawWatchDirPath = rawWatchDir;
			std::filesystem::path changedPathParent = changedMaterialPath.parent_path().parent_path();

			while (!found && !changedPathParent.empty() && changedPathParent.root_path() != changedPathParent)
			{
				if (rawWatchDirPath == changedPathParent)
				{
					found = true;
				}
				else
				{
					changedPathParent = changedPathParent.parent_path();
				}
			}

			if(found)
				break;

			++index;
		}

		if (!found)
		{
			return;
		}

		//we know which index to use now
		std::string manifestPathBin = mManifestBinaryFilePaths[index];
		std::string manifestPathRaw = mManifestRawFilePaths[index];
		std::string materialPackDirRaw = mMaterialPacksWatchDirectoriesRaw[index];
		std::string materialPackDirBin = mMaterialPacksWatchDirectoriesBin[index];


		//delete the mprm associated with this material
	    std::filesystem::path mprmParentDirBinPath = std::filesystem::path(materialPackDirBin) / changedMaterialPath.parent_path().stem();
		const std::string MPRM_EXT = ".mprm";

		for (const auto& file : std::filesystem::directory_iterator(mprmParentDirBinPath))
		{
			if (file.path().extension() == MPRM_EXT)
			{
				std::filesystem::remove(file.path());
			}
		}

		//Then make the mprm again
		GenerateMaterialParamsFromJSON(mprmParentDirBinPath.string(), changedPath);

		//now regenerate the params packs manifest
		bool result = RegenerateMaterialParamsPackManifest(manifestPathBin, manifestPathRaw, materialPackDirBin, materialPackDirRaw);

		if (!result)
		{
			R2_CHECK(false, "We failed to regenerate the material params pack manifest");
			return;
		}

		r2::draw::matsys::MaterialChanged(changedPath);// -- @TODO(Serge): call this when we have the other part done
	}

	void MaterialHotReloadCommand::MaterialAddedRequest(const std::string& newPath)
	{
		r2::draw::matsys::MaterialAdded(newPath);
	}

	void MaterialHotReloadCommand::MaterialRemovedRequest(const std::string& removedPath)
	{
		r2::draw::matsys::MaterialRemoved(removedPath);
	}

}

#endif