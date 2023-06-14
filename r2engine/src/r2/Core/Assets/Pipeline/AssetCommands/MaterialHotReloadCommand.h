#ifndef __MATERIAL_HOT_RELOAD_COMMAND_H__
#define __MATERIAL_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/MaterialPackManifestUtils.h"

namespace r2::asset::pln
{
	class MaterialHotReloadCommand : public AssetHotReloadCommand
	{
	public:
		MaterialHotReloadCommand();
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() override;
		virtual bool ShouldRunOnMainThread() override;
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const override;

		void AddManifestRawFilePaths(const std::vector<std::string>& manifestRawFilePaths);
		void AddManifestBinaryFilePaths(const std::vector<std::string>& manifestBinaryFilePaths);
		void AddRawMaterialPacksWatchDirectories(const std::vector<std::string>& rawWatchPaths);
		void AddBinaryMaterialPacksWatchDirectories(const std::vector<std::string>& binWatchPaths);

		void AddFindMaterialPackManifestFunctions(const std::vector<FindMaterialPackManifestFileFunc>& findFuncs);
		void AddGenerateMaterialPackManifestsFromDirectoriesFunctions(std::vector<GenerateMaterialPackManifestFromDirectoriesFunc> genFuncs);

		static bool MaterialManifestHotReloaded(const char* changedFilePath, const byte* manifestData);


	private:

		void GenerateMaterialPackManifestsIfNeeded();

		void MaterialChangedRequest(const std::string& changedPath);
		void MaterialAddedRequest(const std::string& newPath);
		void MaterialRemovedRequest(const std::string& removedPath);

		s64 FindIndexForPath(const std::string& path, std::string& nameOfMaterial);

		std::vector<std::string> mManifestRawFilePaths;
		std::vector<std::string> mManifestBinaryFilePaths;
		std::vector<std::string> mMaterialPacksWatchDirectoriesRaw;
		std::vector<std::string> mMaterialPacksWatchDirectoriesBin;

		std::vector<FindMaterialPackManifestFileFunc> mFindMaterialFuncs;
		std::vector<GenerateMaterialPackManifestFromDirectoriesFunc> mGenerateMaterialPackFuncs;

	};
}
#endif
#endif
