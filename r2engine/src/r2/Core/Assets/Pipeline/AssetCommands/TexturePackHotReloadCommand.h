#ifndef __TEXTURE_PACK_HOT_RELOAD_COMMAND_H__
#define __TEXTURE_PACK_HOT_RELOAD_COMMAND_H__

#ifdef R2_ASSET_PIPELINE

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

	private:

		void GenerateTexturePackManifestsIfNeeded();
		void TextureChangedRequest(const std::string& changedPath);
		void TextureAddedRequest(const std::string& newPath);
		void TextureRemovedRequest(const std::string& removedPath);



		std::vector<std::string> mManifestRawFilePaths;
		std::vector<std::string> mManifestBinaryFilePaths;
		std::vector<std::string> mTexturePacksWatchDirectories;



		//AssetsBuiltFunc mBuildFunc;
	};
}

#endif
#endif