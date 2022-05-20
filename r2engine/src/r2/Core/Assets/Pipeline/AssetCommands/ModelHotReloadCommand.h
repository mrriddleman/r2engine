#ifndef __MODEL_HOT_RELOAD_COMMAND_H__
#define __MODEL_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"
#include <vector>
#include <string>

namespace r2::asset::pln
{
	class ModelHotReloadCommand : public AssetHotReloadCommand
	{
	public:

		ModelHotReloadCommand();
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override;
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const override;

		void AddBinaryModelDirectories(const std::vector<std::string>& binaryModelDirectories);

	private:
		std::vector<std::string> mBinaryModelDirectories;
	};
}

#endif
#endif