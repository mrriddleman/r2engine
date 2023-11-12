#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/AssetCommands/TexturePackHotReloadCommand.h"
#include "r2/Core/Assets/Pipeline/TexturePackManifestUtils.h"
#include "r2/Render/Model/Textures/TexturePackManifest_generated.h"
#include "assetlib/ImageConvert.h"
#include "r2/Core/Assets/Pipeline/AssetConverterUtils.h"
#include "r2/Core/Assets/AssetLib.h"

#include "r2/Render/Model/Materials/MaterialPack_generated.h"
#include "r2/Render/Model/Materials/MaterialHelpers.h"

#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::asset::pln
{
	void TexturePackHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateRTexFilesIfNeeded();
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

	void TexturePackHotReloadCommand::AddTexturePackBinaryOutputDirectories(const std::vector<std::string>& outputDirectories)
	{
		mTexturePacksBinaryOutputDirectories.insert(mTexturePacksBinaryOutputDirectories.end(), outputDirectories.begin(), outputDirectories.end());
	}

	bool TexturePackHotReloadCommand::TexturePacksManifestHotReloaded(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, r2::asset::HotReloadType type)
	{
		R2_CHECK(manifestData != nullptr, "Should never happen");
		const flat::TexturePacksManifest* texturePacksManifest = flat::GetTexturePacksManifest(manifestData);

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		gameAssetManager.UpdateTexturePacksManifest(r2::asset::Asset::GetAssetNameForFilePath(manifestFilePath.c_str(), r2::asset::TEXTURE_PACK_MANIFEST), texturePacksManifest);

		//I guess try to find the path in question

		for (u32 j = 0; j < paths.size(); ++j)
		{
			std::filesystem::path thePath = paths.at(j);
			
			bool isTexturePack = false;

			if (!std::filesystem::exists(thePath) && type == DELETED)
			{
				//I guess check to see if there's an extension - if so it's a regular file
				if (!thePath.has_extension())
				{
					isTexturePack = true;
				}
			}
			else
			{
				if (std::filesystem::is_directory(thePath))
				{
					isTexturePack = true;
				}
			}

			r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

			r2::SArray<const byte*>* materialManifestDataArray = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const byte*, 100); //dunno how many - I guess we could query the assetLib for it but w/e
			
			r2::asset::lib::GetManifestDataForType(assetLib, MATERIAL_PACK_MANIFEST, materialManifestDataArray);
			std::vector<const flat::Material*> materialsToReload;
			u32 numMaterialManifests = r2::sarr::Size(*materialManifestDataArray);

			if (!isTexturePack)
			{
				//then this must be a singular texture
				//we need to replace the extension since we get the original texture
				thePath.replace_extension(".rtex");
				u64 textureNameFromPath = r2::asset::Asset::GetAssetNameForFilePath(thePath.string().c_str(), TEXTURE);
				u64 texturePackNameFromPath = r2::asset::Asset::GetAssetNameForFilePath(thePath.parent_path().parent_path().stem().string().c_str(), TEXTURE_PACK);

				//we can reload the texture with the gameassetmanager 
				//now we have to figure out all of the materials this is used in
				//once we figure that out, then we can reload the rendermaterials
				if (type != DELETED)
				{
					gameAssetManager.ReloadTextureInTexturePack(texturePackNameFromPath, textureNameFromPath);
				}
				else
				{
					gameAssetManager.UnloadAsset(textureNameFromPath);
				}

				for (u32 i = 0; i < numMaterialManifests; ++i)
				{
					const flat::MaterialPack* materialPack = flat::GetMaterialPack(r2::sarr::At(*materialManifestDataArray, i));
					R2_CHECK(materialPack != nullptr, "Should never happen");
					auto materialParams = r2::mat::GetAllMaterialsInMaterialPackThatContainTexture(materialPack, texturePackNameFromPath, textureNameFromPath);
					materialsToReload.insert(materialsToReload.end(), materialParams.begin(), materialParams.end());
				}
			}
			else
			{
				//this is a texture pack
				u64 texturePackNameFromPath = r2::asset::Asset::GetAssetNameForFilePath(thePath.stem().string().c_str(), TEXTURE_PACK);

				//now we have to figure out all of the materials this texture pack is used in
				//one we figure that out, we can either reload the pack, unload it or load it
				//then we can reload the rendermaterials

				if (type != DELETED)
				{
					gameAssetManager.ReloadTexturePack(texturePackNameFromPath);
				}
				else
				{
					gameAssetManager.UnloadTexturePack(texturePackNameFromPath);
				}

				for (u32 i = 0; i < numMaterialManifests; ++i)
				{
					const flat::MaterialPack* materialPack = flat::GetMaterialPack(r2::sarr::At(*materialManifestDataArray, i));
					R2_CHECK(materialPack != nullptr, "Should never happen");
					auto materialParams = r2::mat::GetAllMaterialsInMaterialPackThatContainTexturePack(materialPack, texturePackNameFromPath);
					materialsToReload.insert(materialsToReload.end(), materialParams.begin(), materialParams.end());
				}

			}

			r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
			R2_CHECK(renderMaterialCache != nullptr, "This should never be nullptr");

			//Now reload the texture using the materialParams
			r2::SArray<r2::draw::tex::Texture>* textures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, r2::draw::tex::Cubemap);
			r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, r2::draw::tex::Cubemap);

			for (const flat::Material* material : materialsToReload)
			{
				bool result = gameAssetManager.GetTexturesForFlatMaterial(material, textures, cubemaps);
				R2_CHECK(result, "Should never be false");

				r2::draw::tex::CubemapTexture* cubemapTextureToUse = nullptr;
				r2::SArray<r2::draw::tex::Texture>* texturesToUse = nullptr;

				if (r2::sarr::Size(*textures) > 0)
				{
					texturesToUse = textures;
				}

				if (r2::sarr::Size(*cubemaps) > 0)
				{
					cubemapTextureToUse = &r2::sarr::At(*cubemaps, 0);
				}

				bool isLoaded = r2::draw::rmat::IsMaterialLoadedOnGPU(*renderMaterialCache, material->assetName());

				if (isLoaded)
				{
					result = r2::draw::rmat::UploadMaterialTextureParams(*renderMaterialCache, material, texturesToUse, cubemapTextureToUse, true);
				}

				r2::sarr::Clear(*textures);
				r2::sarr::Clear(*cubemaps);
			}

			FREE(cubemaps, *MEM_ENG_SCRATCH_PTR);
			FREE(textures, *MEM_ENG_SCRATCH_PTR);
			FREE(materialManifestDataArray, *MEM_ENG_SCRATCH_PTR);
		}

		return false;
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

	s64 TexturePackHotReloadCommand::FindPathIndex(const std::string& newPath, std::string& nameOfPack)
	{
		//We need to check to see if we have a meta file and more than 1 file, if true then we can trigger the add
		s64 index = 0;
		bool found = false;


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
			return -1;

		return index;
	}

	bool TexturePackHotReloadCommand::ConvertImage(const std::string& imagePath, s64 index, std::filesystem::path& outputPath)
	{
		outputPath = r2::asset::pln::tex::GetOutputFilePath(imagePath, mTexturePacksWatchDirectories[index], mTexturePacksBinaryOutputDirectories[index]);

		std::filesystem::path imageChangedPath = imagePath;

		bool result = r2::assets::assetlib::ConvertImage(imageChangedPath, outputPath.parent_path(), imageChangedPath.extension().string(), 1, flat::MipMapFilter_BOX);

		R2_CHECK(result, "We should have converted: %s\n", imagePath.c_str());

		return result;
	}

	void TexturePackHotReloadCommand::TextureChangedRequest(const std::string& changedPath)
	{
		std::string nameOfPack = "";

		s64 index = FindPathIndex(changedPath, nameOfPack);

		std::filesystem::path outputPath;

		ConvertImage(changedPath, index, outputPath);


		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		r2::asset::lib::PathChangedInManifest(assetLib, mManifestBinaryFilePaths[index], { changedPath });
		//r2::draw::matsys::TextureChanged(outputPath.string());
	}

	bool TexturePackHotReloadCommand::HasMetaFileAndAtleast1File(const std::string& watchDir, const std::string& nameOfPack)
	{
		std::filesystem::path packPath = std::filesystem::path(watchDir) / nameOfPack;

		bool hasMetaFile = false;
		bool hasAFile = false;

		if (!std::filesystem::exists(packPath) || !std::filesystem::is_directory(packPath))
			return false;

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

			if (hasMetaFile && hasAFile)
				break;
		}

		return hasAFile && hasMetaFile;
	}

	void TexturePackHotReloadCommand::RegenerateTexturePackManifestFile(s64 index)
	{
		std::string binaryTextureDir = std::filesystem::path(mManifestBinaryFilePaths[index]).parent_path().parent_path().string();

		pln::tex::GenerateTexturePacksManifestFromDirectories(
			mManifestBinaryFilePaths[index],
			mManifestRawFilePaths[index],
			mTexturePacksWatchDirectories[index],
			binaryTextureDir);
	}

	void TexturePackHotReloadCommand::TextureAddedRequest(const std::string& newPath)
	{
		//Check to see if we have a meta file and at least 1 file in one of the sub directories

		std::filesystem::path newlyCreatedPath = newPath;

		if (newlyCreatedPath.filename().string() == "meta.json")
			return;

		std::string nameOfPack = "";
		s64 index = FindPathIndex(newPath, nameOfPack);

		bool result = HasMetaFileAndAtleast1File(mTexturePacksWatchDirectories[index], nameOfPack);

		if (!result)
			return;

		//load the manifest and check to see if we have the texture pack already
		bool hasTexturePackInManifest = r2::asset::pln::tex::HasTexturePackInManifestFile(mManifestBinaryFilePaths[index], nameOfPack);

		bool hasFileAlready = false;
		if (hasTexturePackInManifest)
		{
			//TODO(Serge): newPath should be the .rtex file

			std::filesystem::path outputFilePath = pln::tex::GetOutputFilePath(newPath, mTexturePacksWatchDirectories[index], mTexturePacksBinaryOutputDirectories[index]);

			hasFileAlready = r2::asset::pln::tex::HasTexturePathInManifestFile(mManifestBinaryFilePaths[index], nameOfPack, outputFilePath.string());
		}

		if (hasFileAlready)
		{
			return;
		}

		//Generate the .rtex files
		if (!hasTexturePackInManifest)
		{
			std::filesystem::path inputPackPath = std::filesystem::path(mTexturePacksWatchDirectories[index]) / nameOfPack;
			std::filesystem::path outputDir = pln::tex::GetOutputFilePath(inputPackPath, mTexturePacksWatchDirectories[index], mTexturePacksBinaryOutputDirectories[index]);

			if (!std::filesystem::exists(outputDir))
			{
				std::filesystem::create_directories(outputDir);
			}

			int result = pln::assetconvert::RunConverter(inputPackPath.string(), outputDir.string());

			if (result != 0)
			{
				R2_CHECK(false, "Converter failed!");
			}
		}
		else
		{
			std::filesystem::path outputFilePath;
			bool result = ConvertImage(newPath, index, outputFilePath);

			R2_CHECK(result, "Couldn't convert: %s\n", newPath.c_str());
		}

		//We need to regenerate the manifest file
		RegenerateTexturePackManifestFile(index);


		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		if (!hasTexturePackInManifest)
		{
			std::filesystem::path packPath = std::filesystem::path(mTexturePacksWatchDirectories[index]) / nameOfPack;

			//std::vector<std::vector<std::string>> pathsInTexturePack = pln::tex::GetAllTexturesInTexturePack(mManifestBinaryFilePaths[index], nameOfPack);

			
			

			r2::asset::lib::PathAddedInManifest(assetLib, mManifestBinaryFilePaths[index], { packPath.string() });

			//r2::draw::matsys::TexturePackAdded(mManifestBinaryFilePaths[index], packPath.string(), pathsInTexturePack);
		}
		else
		{

			r2::asset::lib::PathAddedInManifest(assetLib, mManifestBinaryFilePaths[index], { newPath });
			//r2::draw::matsys::TextureAdded(mManifestBinaryFilePaths[index], newPath);
		}
	}

	void TexturePackHotReloadCommand::TextureRemovedRequest(const std::string& removedPathStr)
	{
		std::string nameOfPack = "";
		s64 index = FindPathIndex(removedPathStr, nameOfPack);

		//@NOTE(Serge): this might break if we have 2 of the same name?

		bool hasTexturePackInManifest = r2::asset::pln::tex::HasTexturePackInManifestFile(mManifestBinaryFilePaths[index], nameOfPack);

		if (!hasTexturePackInManifest)
		{
			return;
		}

		bool hasMetaAndAtleast1File = HasMetaFileAndAtleast1File(mTexturePacksWatchDirectories[index], nameOfPack);

		std::filesystem::path removedPath = std::filesystem::path(removedPathStr);
		std::filesystem::path packPath = std::filesystem::path(mTexturePacksWatchDirectories[index]) / nameOfPack;
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		if (!hasMetaAndAtleast1File)
		{
			//std::vector<std::vector<std::string>> pathsLeftInTexturePack = pln::tex::GetAllTexturesInTexturePack(mManifestBinaryFilePaths[index], nameOfPack);
			
			std::filesystem::path outputPackPath = pln::tex::GetOutputFilePath(packPath, mTexturePacksWatchDirectories[index], mTexturePacksBinaryOutputDirectories[index]);

			RegenerateTexturePackManifestFile(index);

			r2::asset::lib::PathRemovedInManifest(assetLib, mManifestBinaryFilePaths[index], { packPath.string() });
			//r2::draw::matsys::TexturePackRemoved(mManifestBinaryFilePaths[index], packPath.string(), pathsLeftInTexturePack);
			return;
		}

		//TODO(Serge): removedPathStr should be the .rtex file

		std::filesystem::path outputFilePath = pln::tex::GetOutputFilePath(removedPath, mTexturePacksWatchDirectories[index], mTexturePacksBinaryOutputDirectories[index]);

		bool hasTexturePathInManifest = r2::asset::pln::tex::HasTexturePathInManifestFile(mManifestBinaryFilePaths[index], nameOfPack, outputFilePath.string());

		if (hasTexturePathInManifest)
		{
			RegenerateTexturePackManifestFile(index);

			r2::asset::lib::PathRemovedInManifest(assetLib, mManifestBinaryFilePaths[index], { outputFilePath.string() });

		//	r2::draw::matsys::TextureRemoved(mManifestBinaryFilePaths[index], outputFilePath.string());
		}
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> TexturePackHotReloadCommand::DirectoriesToCreate() const
	{
		CreateDirCmd createDirCMD;
		createDirCMD.pathsToCreate = mManifestBinaryFilePaths;
		createDirCMD.startAtOne = true;
		createDirCMD.startAtParent = true;

		CreateDirCmd createDirCMD2;
		createDirCMD2.pathsToCreate = mTexturePacksBinaryOutputDirectories;
		createDirCMD2.startAtOne = false;
		createDirCMD2.startAtParent = false;


		return { createDirCMD, createDirCMD2 };
	}

	void TexturePackHotReloadCommand::GenerateRTexFilesIfNeeded()
	{
		R2_CHECK(mTexturePacksWatchDirectories.size() == mTexturePacksBinaryOutputDirectories.size(), "These should be the same");

		const u64 numWatchPaths = mTexturePacksWatchDirectories.size();

		for (u64 i = 0; i < numWatchPaths; ++i)
		{
			std::filesystem::path inputDirPath = mTexturePacksWatchDirectories[i];
			inputDirPath.make_preferred();
			std::filesystem::path outputDirPath = mTexturePacksBinaryOutputDirectories[i];
			outputDirPath.make_preferred();

			if (std::filesystem::is_empty(outputDirPath))
			{
				int result = pln::assetconvert::RunConverter(inputDirPath.string(), outputDirPath.string());
				R2_CHECK(result == 0, "Failed to create the rtex files of: %s\n", inputDirPath.string().c_str());
			}
			else
			{
				//for now I guess we'll just check to see if we have an equivalent directory in the sub directories (and if they aren't empty
				//if not then run the converter for that directory, otherwise skip. Unfortunately, this will not catch the case of individual files not being converted...
				for (const auto& p : std::filesystem::directory_iterator(inputDirPath))
				{
					std::filesystem::path outputPath = pln::tex::GetOutputFilePath(p.path(), mTexturePacksWatchDirectories[i], mTexturePacksBinaryOutputDirectories[i]);
					outputPath.make_preferred();

					if (!std::filesystem::exists(outputPath))
					{
						std::filesystem::create_directory(outputPath);
					}

					if (std::filesystem::is_empty(outputPath))
					{
						int result = pln::assetconvert::RunConverter(p.path().string(), outputPath.string());

						R2_CHECK(result == 0, "Failed to create the rtex files of: %s\n", p.path().string().c_str());
					}
				}
			}
		}
	}
}

#endif