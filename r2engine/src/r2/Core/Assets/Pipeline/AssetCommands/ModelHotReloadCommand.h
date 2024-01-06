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
		void AddRawModelDirectories(const std::vector<std::string>& rawModelDirectories);

		void AddMaterialManifestPaths(const std::vector<std::string>& materialManifestPaths);
		void AddTextureManifestPaths(const std::string& engineTexturePacksManifestPath, const std::string& appTexturePacksManifestPath);
		void AddMaterialRawDirectory(const std::string& materialRawDirectory);

		void AddBinaryModelManifestPaths(const std::vector<std::string>& binaryManifestPaths);
		void AddRawModelManifestPaths(const std::vector<std::string>& rawManifestPaths);

		virtual AssetHotReloadCommandType GetAssetHotReloadCommandType() const override { return AHRCT_MODEL_ASSET; }
		virtual void HandleAssetBuildRequest(const AssetBuildRequest& request) override;

	private:

		void GenerateModelsManifestsIfNeeded();
		void GenerateRMDLFilesIfNeeded();
		static std::filesystem::path GetOutputFilePath(const std::filesystem::path& inputPath, const std::filesystem::path& inputPathRootDir, const std::filesystem::path& outputDir);

		std::vector<std::string> mRawModelDirectories;
		std::vector<std::string> mBinaryModelDirectories;
		std::vector<std::string> mMaterialManifestPaths;

		std::vector<std::string> mBinaryModelManifestPaths;
		std::vector<std::string> mRawModelManifestPaths;

		std::string mEngineTexturePacksManifestPath;
		std::string mAppTexturePacksManifestPath;
		std::string mMaterialRawDirectoryPath;
	};
}

#endif
#endif