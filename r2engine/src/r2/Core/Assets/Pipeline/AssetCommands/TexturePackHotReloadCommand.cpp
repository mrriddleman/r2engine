#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/TexturePackHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"

#include "r2/Render/Model/Material.h"

namespace r2::asset::pln
{
	void TexturePackHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateTexturePackManifestsIfNeeded();

		for (const auto& path : mTexturePacksWatchDirectories)
		{
			FileWatcher fw;
			fw.Init(delay, path);

			fw.AddModifyListener(std::bind(&TexturePackHotReloadCommand::TextureChangedRequest, this, std::placeholders::_1));
			fw.AddCreatedListener(std::bind(&TexturePackHotReloadCommand::TextureAddedRequest, this, std::placeholders::_1));

			mFileWatchers.push_back(fw);
		}
	}

	void TexturePackHotReloadCommand::Update()
	{
		for (auto& fw : mFileWatchers)
		{
			fw.Run();
		}
	}

	void TexturePackHotReloadCommand::AddRawManifestFilePaths(const std::vector<std::string>& rawFilePaths)
	{
		mManifestRawFilePaths.insert(mManifestRawFilePaths.end(), rawFilePaths.begin(), rawFilePaths.end());
	}

	void TexturePackHotReloadCommand::AddBinaryManifestFilePaths(const std::vector<std::string>& binFilePaths)
	{
		mManifestBinaryFilePaths.insert(mManifestBinaryFilePaths.end(), binFilePaths.begin(), binFilePaths.end());
	}

	void TexturePackHotReloadCommand::AddTexturePackWatchDirectories(const std::vector<std::string>& watchPaths)
	{
		mTexturePacksWatchDirectories.insert(mTexturePacksWatchDirectories.end(), watchPaths.begin(), watchPaths.end());
	}

	//Private methods
	void TexturePackHotReloadCommand::GenerateTexturePackManifestsIfNeeded()
	{
		R2_CHECK(mManifestBinaryFilePaths.size() == mManifestRawFilePaths.size(),
			"these should be the same size!");

		for (size_t i = 0; i < mManifestBinaryFilePaths.size(); ++i)
		{
			std::string manifestBinaryPath = mManifestBinaryFilePaths[i];
			std::string manifestRawPath = mManifestRawFilePaths[i];
			std::string texturePackDir = mTexturePacksWatchDirectories[i];

			std::filesystem::path binaryPath = manifestBinaryPath;
			std::string manifestBinaryDir = binaryPath.parent_path().string();
			std::string binaryTextureDir = binaryPath.parent_path().parent_path().string();

			std::filesystem::path rawPath = manifestRawPath;
			std::string manifestRawDir = rawPath.parent_path().string();

			std::string texturePackManifestFile;
			r2::asset::pln::tex::FindTexturePacksManifestFile(manifestBinaryDir, binaryPath.stem().string(), texturePackManifestFile, true);

			if (texturePackManifestFile.empty())
			{
				if (r2::asset::pln::tex::FindTexturePacksManifestFile(manifestRawDir, rawPath.stem().string(), texturePackManifestFile, false))
				{
					if (!std::filesystem::remove(texturePackManifestFile))
					{
						R2_CHECK(false, "Failed to delete texture pack manifest file from json!");
						return;
					}
				}

				if (!r2::asset::pln::tex::GenerateTexturePacksManifestFromDirectories(binaryPath.string(), rawPath.string(), texturePackDir, binaryTextureDir))
				{
					R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
					return;
				}
			}
		}
	}

	void TexturePackHotReloadCommand::TextureChangedRequest(const std::string& changedPath)
	{
		r2::draw::matsys::TextureChanged(changedPath);
	}

	void TexturePackHotReloadCommand::TextureAddedRequest(const std::string& newPath)
	{
		r2::draw::matsys::TextureAdded(newPath);
	}

	r2::asset::pln::AssetHotReloadCommand::CreateDirCmd TexturePackHotReloadCommand::DirectoriesToCreate() const
	{
		CreateDirCmd createDirCMD;
		createDirCMD.pathsToCreate = mManifestBinaryFilePaths;
		createDirCMD.startAtOne = true;
		createDirCMD.startAtParent = true;

		return createDirCMD;
	}

}

#endif