#ifndef __SHADER_HOT_RELOAD_COMMAND_H__
#define __SHADER_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/ShaderManifest.h"

namespace r2::asset::pln
{
	using InternalShaderPassesBuildFunc = std::function<bool(const std::string& outputDir)>;

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

		void AddInternalShaderPassesBuildDescription(const std::string& rawManifestPath, const std::string& binManifestPath, InternalShaderPassesBuildFunc func);

	private:

		struct InternalShaderPassesBuildData
		{
			std::string internalRawShaderManifestPath;
			std::string internalBinShaderManifestPath;
			InternalShaderPassesBuildFunc func;
		};

		void ReloadShaderManifests();
		void ShaderChangedRequest(const std::string& changedPath);
		void BuildInternalShaderPassesIfNeeded();


		std::vector<std::string> mWatchPaths;
		std::vector<std::string> mManifestFilePaths;

		std::vector<std::vector<ShaderManifest>> mShaderManifests;

		std::vector<InternalShaderPassesBuildData> mInternalShaderPassesBuildData;

	};
}

#endif
#endif