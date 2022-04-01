#ifndef __ASSET_COMMAND_WATCHER_H__
#define __ASSET_COMMAND_WATCHER_H__

#ifdef R2_ASSET_PIPELINE
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <chrono>
#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"

namespace r2::asset::pln
{
	class AssetCommandHandler
	{
	public:

		AssetCommandHandler();
		~AssetCommandHandler() {}
		void Init(
			Milliseconds delay,
			std::vector<std::unique_ptr<AssetHotReloadCommand>> assetCommands);

		void Update();

		void Shutdown();

	private:
		void ThreadProc();
		void MakeGameBinaryAssetFolders() const;

		std::vector<std::unique_ptr<AssetHotReloadCommand>> mAssetCommands;

		std::chrono::steady_clock::time_point mLastTime;
		Milliseconds mDelay;
		std::thread mAssetWatcherThread;
		std::atomic_bool mEnd;
	};
}

#endif
#endif