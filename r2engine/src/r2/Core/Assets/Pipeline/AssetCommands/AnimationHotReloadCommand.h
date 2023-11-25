#ifndef __ANIMATION_HOT_RELOAD_COMMAND_H__
#define __ANIMATION_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"
#include <vector>
#include <string>

namespace r2::asset::pln
{
	class AnimationHotReloadCommand : public AssetHotReloadCommand
	{
	public:

		AnimationHotReloadCommand();
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override;
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const override;

		void AddBinaryAnimationDirectories(const std::vector<std::string>& binaryAnimationDirectories);
		void AddRawAnimationDirectories(const std::vector<std::string>& rawAnimationDirectories);

		virtual AssetHotReloadCommandType GetAssetHotReloadCommandType() const override { return AHRCT_ANIMATION_ASSET; }
	private:
		static std::filesystem::path GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir);
		void GenerateRANMFilesIfNeeded();

		std::vector<std::string> mBinaryAnimationDirectories;
		std::vector<std::string> mRawAnimationDirectories;
	};
}


#endif
#endif