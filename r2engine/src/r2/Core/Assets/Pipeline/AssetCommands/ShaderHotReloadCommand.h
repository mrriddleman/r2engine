#ifndef __SHADER_HOT_RELOAD_COMMAND_H__
#define __SHADER_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest.h"

namespace r2::asset::pln
{
	class ShaderHotReloadCommand : public AssetHotReloadCommand
	{
	public:
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override { return true; }
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const override;

		void AddShaderWatchPaths(const std::vector<std::string>& watchPaths);
		void AddManifestFilePaths(const std::vector<std::string>& manifestFilePaths);

	private:

		void ReloadShaderManifests();
		void ShaderChangedRequest(const std::string& changedPath);

		std::vector<std::string> mWatchPaths;
		std::vector<std::string> mManifestFilePaths;

		std::vector<std::vector<ShaderManifest>> mShaderManifests;
	};
}

#endif
#endif