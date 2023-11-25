#ifndef __TEXTURE_PACK_HOT_RELOAD_COMMAND_H__
#define __TEXTURE_PACK_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Core/Assets/Pipeline/AssetCommands/AssetHotReloadCommand.h"

namespace r2::asset::pln
{
	class TexturePackHotReloadCommand : public AssetHotReloadCommand
	{
	public:
		virtual void Init(Milliseconds delay) override;
		virtual void Update() override;
		virtual void Shutdown() {};
		virtual bool ShouldRunOnMainThread() override { return false; }
		virtual std::vector<CreateDirCmd> DirectoriesToCreate() const override;

		void AddRawManifestFilePaths(const std::vector<std::string>& rawFilePaths);
		void AddBinaryManifestFilePaths(const std::vector<std::string>& binFilePaths);
		void AddTexturePackWatchDirectories(const std::vector<std::string>& watchPaths);
		void AddTexturePackBinaryOutputDirectories(const std::vector<std::string>& outputDirectories);


		static bool TexturePacksManifestHotReloaded(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, HotReloadType type);

		
		virtual AssetHotReloadCommandType GetAssetHotReloadCommandType() const override { return AHRCT_TEXTURE_ASSET; }

		virtual void HandleAssetBuildRequest(const AssetBuildRequest& request) override;
	private:

		void GenerateTexturePackManifestsIfNeeded();
		void TextureChangedRequest(const std::string& changedPath);
		void TextureAddedRequest(const std::string& newPath);

		void AddNewTexturePack(s64 index, const std::string& nameOfPack, const std::string& newPath);

		void TextureRemovedRequest(const std::string& removedPath);

		s64 FindPathIndex(const std::string& newPath, std::string& nameOfPack);
		bool HasMetaFileAndAtleast1File(const std::string& watchDir, const std::string& nameOfPack);
		void RegenerateTexturePackManifestFile(s64 index);

		bool ConvertImage(const std::string& imagePath, s64 index, std::filesystem::path& outputPath);

		void GenerateRTexFilesIfNeeded();

		std::vector<std::string> mManifestRawFilePaths;
		std::vector<std::string> mManifestBinaryFilePaths;
		std::vector<std::string> mTexturePacksWatchDirectories;
		std::vector<std::string> mTexturePacksBinaryOutputDirectories;


		
	};
}

#endif
#endif