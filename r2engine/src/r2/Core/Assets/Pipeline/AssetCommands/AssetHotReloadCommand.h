#ifndef __ASSET_HOT_RELOAD_COMMAND_H__
#define __ASSET_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/FileWatcher.h"

namespace r2::asset::pln
{
	class AssetHotReloadCommand
	{
	public:

		struct CreateDirCmd
		{
			std::vector<std::string> pathsToCreate = {};
			bool startAtOne = false;
			bool startAtParent = false;
		};

		virtual ~AssetHotReloadCommand() {}
		virtual void Init(Milliseconds delay) = 0;
		virtual void Update() = 0;
		virtual void Shutdown() = 0;
		virtual bool ShouldRunOnMainThread() = 0;
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const = 0;

	protected:
		std::vector<FileWatcher> mFileWatchers;
	};
}

#endif
#endif