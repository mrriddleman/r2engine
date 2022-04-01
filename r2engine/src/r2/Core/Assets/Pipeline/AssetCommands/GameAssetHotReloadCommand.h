#ifndef __GAME_ASSET_HOT_RELOAD_COMMAND_H__
#define __GAME_ASSET_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/AssetManifest.h"

namespace r2::asset::pln
{
	class GameAssetHotReloadCommand : public AssetHotReloadCommand
	{
	public:

		GameAssetHotReloadCommand();
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override;
		virtual CreateDirCmd DirectoriesToCreate() const override;

		void SetAssetManifestsPath(const std::string& assetManifestPath);
		void SetAssetTempPath(const std::string& assetTempPath);
		void AddWatchPaths(const std::vector<std::string>& watchPaths);

	private:
		void ReloadManifests();
		void BuildManifests();
		void PushBuildRequest(std::string changedPath);
		void SetReloadManifests(std::string changedPath);

		void ModifiedWatchPathDispatch(std::string path);
		void CreatedWatchPathDispatch(std::string path);
		void RemovedWatchPathDispatch(std::string path);
		

		std::vector<AssetManifest> mManifests;
		std::string mAssetManifestsPath;
		std::string mAssetTempPath;
		std::vector<std::string> mPathsToWatch;

		bool mReloadManifests;
	};
}

#endif
#endif