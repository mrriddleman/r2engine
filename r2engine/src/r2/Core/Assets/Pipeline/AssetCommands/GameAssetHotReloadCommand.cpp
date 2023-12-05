#include "r2pch.h"

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/GameAssetHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/AssetCompiler.h"
#include "r2/Core/Assets/AssetLib.h"

namespace r2::asset::pln
{
	
	GameAssetHotReloadCommand::GameAssetHotReloadCommand()
		: mReloadManifests(false)
	{
	}

	void GameAssetHotReloadCommand::Init(Milliseconds delay)
	{
		ReloadManifests();

	//	r2::asset::pln::cmp::Init(mAssetTempPath, r2::asset::lib::PushFilesBuilt);

		for (const auto& manifest : mManifests)
		{
			r2::asset::pln::cmp::CompileAsset(manifest);
		}

		for (const auto& path : mPathsToWatch)
		{
			FileWatcher fw;
			fw.Init(delay, path);
			fw.AddModifyListener(std::bind(&GameAssetHotReloadCommand::ModifiedWatchPathDispatch, this, std::placeholders::_1) );
			fw.AddCreatedListener(std::bind(&GameAssetHotReloadCommand::CreatedWatchPathDispatch, this, std::placeholders::_1));
			fw.AddRemovedListener(std::bind(&GameAssetHotReloadCommand::RemovedWatchPathDispatch, this, std::placeholders::_1));

			mFileWatchers.push_back(fw);
		}

		FileWatcher fw;
		fw.Init(delay, mAssetManifestsPath);

		fw.AddCreatedListener(std::bind(&GameAssetHotReloadCommand::SetReloadManifests, this, std::placeholders::_1));
		fw.AddModifyListener(std::bind(&GameAssetHotReloadCommand::SetReloadManifests, this, std::placeholders::_1));
		fw.AddRemovedListener(std::bind(&GameAssetHotReloadCommand::SetReloadManifests, this, std::placeholders::_1));

		mFileWatchers.push_back(fw);
	}

	void GameAssetHotReloadCommand::Update()
	{
		for (auto& fileWatcher : mFileWatchers)
		{
			fileWatcher.Run();
		}

		if (mReloadManifests)
		{
			ReloadManifests();
		}

		//pull commands from the queue to request asset builds
		r2::asset::pln::cmp::Update();
	}

	bool GameAssetHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> GameAssetHotReloadCommand::DirectoriesToCreate() const
	{
		return {};
	}

	void GameAssetHotReloadCommand::SetAssetManifestsPath(const std::string& assetManifestPath)
	{
		mAssetManifestsPath = assetManifestPath;
	}

	void GameAssetHotReloadCommand::SetAssetTempPath(const std::string& assetTempPath)
	{
		mAssetTempPath = assetTempPath;
	}

	void GameAssetHotReloadCommand::AddWatchPaths(const std::vector<std::string>& watchPaths)
	{
		mPathsToWatch.insert(mPathsToWatch.end(), watchPaths.begin(), watchPaths.end());
	}

	void GameAssetHotReloadCommand::ReloadManifests()
	{
		BuildManifests();

		LoadAssetManifests(mManifests, mAssetManifestsPath);
		mReloadManifests = false;
	}

	void GameAssetHotReloadCommand::BuildManifests()
	{
		BuildAssetManifestsFromJson(mAssetManifestsPath);
	}

	void GameAssetHotReloadCommand::ModifiedWatchPathDispatch(std::string path)
	{
		PushBuildRequest(path);
	}

	void GameAssetHotReloadCommand::CreatedWatchPathDispatch(std::string path)
	{
		PushBuildRequest(path);
	}

	void GameAssetHotReloadCommand::RemovedWatchPathDispatch(std::string path)
	{

	}

	void GameAssetHotReloadCommand::PushBuildRequest(std::string changedPath)
	{
		for (const auto& manifest : mManifests)
		{
			for (const auto& fileCommand : manifest.fileCommands)
			{
				if (std::filesystem::path(fileCommand.inputPath) == std::filesystem::path(changedPath))
				{
					r2::asset::pln::cmp::PushBuildRequest(manifest);
					return;
				}
			}
		}
	}

	void GameAssetHotReloadCommand::SetReloadManifests(std::string changedPath)
	{
		mReloadManifests = true;
	}
}


#endif