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
			fw.AddRemovedListener(std::bind(&TexturePackHotReloadCommand::TextureRemovedRequest, this, std::placeholders::_1));

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
		//We could have different methods that will add a texture pack instead of an individual file
		//We could check the parent path, and figure out the manifest and see if we have it already in the manifest - if we don't then add the pack, otherwise call TextureAdded()

		//We need to check to see if we have a meta file and more than 1 file, if true then we can trigger the add
		size_t index = 0;
		bool found = false;
		std::string nameOfPack = "";

		for (const std::string& rawWatchDir : mTexturePacksWatchDirectories)
		{
			std::filesystem::path addedPath = newPath;

			nameOfPack = addedPath.parent_path().stem().string();

			std::filesystem::path rawWatchPath = rawWatchDir;

			while (!found && !addedPath.empty() && addedPath.root_path() != addedPath)
			{
				if (rawWatchPath == addedPath)
				{
					found = true;
				}
				else
				{
					nameOfPack = addedPath.stem().string();
					addedPath = addedPath.parent_path();
				}
			}

			if (found)
				break;

			++index;
		}

		if (!found)
			return;

		//Check to see if we have a meta file and at least 1 file in one of the sub directories
		
		std::filesystem::path packPath = std::filesystem::path(mTexturePacksWatchDirectories[index]) / nameOfPack;

		bool hasMetaFile = false;
		bool hasAFile = false;
		for (const auto& packItem : std::filesystem::directory_iterator(packPath))
		{
			if (!packItem.is_directory() && packItem.file_size() > 0 && packItem.path().stem().string() == "meta")
			{
				hasMetaFile = true;
			}
			else if (packItem.is_directory())
			{
				for (const auto& dirItem : std::filesystem::directory_iterator(packItem))
				{
					if (dirItem.is_regular_file() && dirItem.file_size() > 0)
					{
						hasAFile = true;
						break;
					}
				}
			}

			if(hasMetaFile && hasAFile)
				break;
		}


		if (!hasAFile || !hasMetaFile)
			return;


		//load the manifest and check to see if we have the texture pack already
		bool hasTexturePackInManifest = r2::asset::pln::tex::HasTexturePackInManifestFile(mManifestBinaryFilePaths[index], nameOfPack);

		bool hasFileAlready = false;
		if (hasTexturePackInManifest)
		{
			hasFileAlready = r2::asset::pln::tex::HasTexturePathInManifestFile(mManifestBinaryFilePaths[index], nameOfPack, newPath);
		}

		if (hasFileAlready)
		{
			return;
		}

		//We need to regenerate the manifest file
		std::string binaryTextureDir = std::filesystem::path(mManifestBinaryFilePaths[index]).parent_path().parent_path().string();


		pln::tex::GenerateTexturePacksManifestFromDirectories(
			mManifestBinaryFilePaths[index],
			mManifestRawFilePaths[index],
			mTexturePacksWatchDirectories[index],
			binaryTextureDir);


		if (!hasTexturePackInManifest)
		{
			r2::draw::matsys::TexturePackAdded(mManifestBinaryFilePaths[index]);
		}
		else
		{
			r2::draw::matsys::TextureAdded(mManifestBinaryFilePaths[index]);
		}
	}

	void TexturePackHotReloadCommand::TextureRemovedRequest(const std::string& removedPath)
	{
		r2::draw::matsys::TextureRemoved(removedPath);
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> TexturePackHotReloadCommand::DirectoriesToCreate() const
	{
		CreateDirCmd createDirCMD;
		createDirCMD.pathsToCreate = mManifestBinaryFilePaths;
		createDirCMD.startAtOne = true;
		createDirCMD.startAtParent = true;

		return { createDirCMD };
	}

}

#endif