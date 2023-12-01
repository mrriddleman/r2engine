#include "r2pch.h"
#ifdef R2_ASSET_PIPELINE

#include "r2/Core/Assets/Pipeline/AssetCommands/MaterialHotReloadCommand.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Render/Model/Materials/MaterialPack_generated.h"
#include "r2/Utils/Hash.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"
#include "r2/Render/Model/Textures/Texture.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"

namespace
{
	const std::string MTRL_EXT = ".mtrl";
}

namespace r2::asset::pln
{

	MaterialHotReloadCommand::MaterialHotReloadCommand()
	{

	}

	void MaterialHotReloadCommand::Init(Milliseconds delay)
	{
		GenerateMaterialPackManifestsIfNeeded();

		for (const auto& path : mMaterialPacksWatchDirectoriesRaw)
		{
			FileWatcher fw;
			fw.Init(delay, path);

			fw.AddModifyListener(std::bind(&MaterialHotReloadCommand::MaterialChangedRequest, this, std::placeholders::_1));
			fw.AddCreatedListener(std::bind(&MaterialHotReloadCommand::MaterialAddedRequest, this, std::placeholders::_1));
			fw.AddRemovedListener(std::bind(&MaterialHotReloadCommand::MaterialRemovedRequest, this, std::placeholders::_1));

			mFileWatchers.push_back(fw);
		}
	}

	void MaterialHotReloadCommand::Update()
	{
		for (auto& fw : mFileWatchers)
		{
			fw.Run();
		}
	}

	void MaterialHotReloadCommand::Shutdown()
	{

	}

	bool MaterialHotReloadCommand::ShouldRunOnMainThread()
	{
		return false;
	}

	std::vector<r2::asset::pln::AssetHotReloadCommand::CreateDirCmd> MaterialHotReloadCommand::DirectoriesToCreate() const
	{
		CreateDirCmd createBinaryManifestDirCmd;
		createBinaryManifestDirCmd.pathsToCreate = mManifestBinaryFilePaths;
		createBinaryManifestDirCmd.startAtOne = true;
		createBinaryManifestDirCmd.startAtParent = true;

		CreateDirCmd createWatchDirBinCmd;
		createWatchDirBinCmd.pathsToCreate = mMaterialPacksWatchDirectoriesBin;
		createWatchDirBinCmd.startAtOne = false;
		createWatchDirBinCmd.startAtParent = false;

		return {createBinaryManifestDirCmd, createWatchDirBinCmd};
	}

	void MaterialHotReloadCommand::AddManifestRawFilePaths(const std::vector<std::string>& manifestRawFilePaths)
	{
		mManifestRawFilePaths.insert(mManifestRawFilePaths.end(), manifestRawFilePaths.begin(), manifestRawFilePaths.end());
	}

	void MaterialHotReloadCommand::AddManifestBinaryFilePaths(const std::vector<std::string>& manifestBinaryFilePaths)
	{
		mManifestBinaryFilePaths.insert(mManifestBinaryFilePaths.end(), manifestBinaryFilePaths.begin(), manifestBinaryFilePaths.end());
	}

	void MaterialHotReloadCommand::AddRawMaterialPacksWatchDirectories(const std::vector<std::string>& rawWatchPaths)
	{
		mMaterialPacksWatchDirectoriesRaw.insert(mMaterialPacksWatchDirectoriesRaw.end(), rawWatchPaths.begin(), rawWatchPaths.end());
	}

	void MaterialHotReloadCommand::AddBinaryMaterialPacksWatchDirectories(const std::vector<std::string>& binWatchPaths)
	{
		mMaterialPacksWatchDirectoriesBin.insert(mMaterialPacksWatchDirectoriesBin.end(), binWatchPaths.begin(), binWatchPaths.end());
	}

	void MaterialHotReloadCommand::AddFindMaterialPackManifestFunctions(const std::vector<FindMaterialPackManifestFileFunc>& findFuncs)
	{
		for (auto& func : findFuncs)
		{
			mFindMaterialFuncs.push_back(func);
		}
	}

	void MaterialHotReloadCommand::AddGenerateMaterialPackManifestsFromDirectoriesFunctions(std::vector<GenerateMaterialPackManifestFromDirectoriesFunc> genFuncs)
	{
		for (auto& func : genFuncs)
		{
			mGenerateMaterialPackFuncs.push_back(func);
		}
	}

	bool MaterialHotReloadCommand::MaterialManifestHotReloaded(const std::vector<std::string>& paths, const std::string& manifestFilePath, const byte* manifestData, HotReloadType type)
	{
		//first find the material if it exists
		//if it doesn't then it probably was removed so unload it
		//const flat::MaterialParamsPack* materialParamsPack = flat::GetMaterialParamsPack(manifestData);
		const flat::MaterialPack* materialPack = flat::GetMaterialPack(manifestData);


		//@TODO(Serge): this is how it should be I think
		//u64 materialName = r2::asset::Asset::GetAssetNameForFilePath(changedFilePath, r2::asset::MATERIAL);

		R2_CHECK(paths.size() == 1, "Always should be the case");
		//For now though, I think we need to do this since we're not guarenteed that our names are all lowercase (as GetAssetNameForFilePath would make)
		std::filesystem::path changedPath = paths[0];
		std::string materialNameStr = changedPath.stem().string();
		u64 materialName = STRING_ID(materialNameStr.c_str());

		r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
		R2_CHECK(renderMaterialCache != nullptr, "This should never be nullptr");

		bool isLoaded = r2::draw::rmat::IsMaterialLoadedOnGPU(*renderMaterialCache, materialName);

		//now find it in the MaterialParamsPack. If we don't find it then, we know it was removed
		//const flat::MaterialParams* foundMaterialParams = nullptr;
		const flat::Material* foundMaterial = nullptr;

		//const auto numMaterialParams = materialParamsPack->pack()->size();
		const auto* materials = materialPack->pack();
		const auto numMaterials = materialPack->pack()->size();

		for (flatbuffers::uoffset_t i = 0; i < numMaterials; ++i)
		{
			//const auto* materialParams = materialParamsPack->pack()->Get(i);

			const auto* material = materials->Get(i);

			if (material->assetName() == materialName)//materialParams->name() == materialName)
			{
				//foundMaterialParams = materialParams;
				foundMaterial = material;
				break;
			}
		}

		if (foundMaterial)
		{
			//load the material params again
			r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

			r2::SArray<r2::draw::tex::Texture>* textures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, 200);
			r2::SArray<r2::draw::tex::CubemapTexture>* cubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, r2::draw::tex::NUM_TEXTURE_TYPES);
			bool result = gameAssetManager.GetTexturesForFlatMaterial(foundMaterial, textures, cubemaps);
			R2_CHECK(result, "This should always work");

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

			if (isLoaded)
			{
				gameAssetManager.LoadMaterialTextures(foundMaterial); //@NOTE(Serge): this actually does nothing at the moment since this pack is probably loaded already
				result = r2::draw::rmat::UploadMaterialTextureParams(*renderMaterialCache, foundMaterial, texturesToUse, cubemapTextureToUse, true);
				R2_CHECK(result, "This should always work");
			}

			FREE(cubemaps, *MEM_ENG_SCRATCH_PTR);
			FREE(textures, *MEM_ENG_SCRATCH_PTR);
		}
		else
		{
			r2::draw::rmat::UnloadMaterial(*renderMaterialCache, materialName);
		}

		return true;
	}

	void MaterialHotReloadCommand::GenerateMaterialPackManifestsIfNeeded()
	{
		R2_CHECK(mManifestBinaryFilePaths.size() == mManifestRawFilePaths.size(),
			"these should be the same size!");

		R2_CHECK(mFindMaterialFuncs.size() == mManifestBinaryFilePaths.size(), "these should be the same size");
		R2_CHECK(mGenerateMaterialPackFuncs.size() == mManifestBinaryFilePaths.size(), "these should be the same size");

		for (size_t i = 0; i < mManifestBinaryFilePaths.size(); ++i)
		{
			std::string manifestPathBin = mManifestBinaryFilePaths[i];
			std::string manifestPathRaw = mManifestRawFilePaths[i];
			std::string materialPackDirRaw = mMaterialPacksWatchDirectoriesRaw[i];
			std::string materialPackDirBin = mMaterialPacksWatchDirectoriesBin[i];

			std::filesystem::path binaryPath = manifestPathBin;
			std::string binManifestDir = binaryPath.parent_path().string();

			std::filesystem::path rawPath = manifestPathRaw;
			std::string rawManifestDir = rawPath.parent_path().string();

			std::string materialPackManifestFile;

			FindMaterialPackManifestFileFunc FindFunc = mFindMaterialFuncs[i];
			GenerateMaterialPackManifestFromDirectoriesFunc GenerateFromDirFunc = mGenerateMaterialPackFuncs[i];

			FindFunc(binManifestDir, binaryPath.stem().string(), materialPackManifestFile, true);

			if (materialPackManifestFile.empty())
			{
				if (FindFunc(rawManifestDir, rawPath.stem().string(), materialPackManifestFile, false))
				{
					if (!std::filesystem::remove(materialPackManifestFile))
					{
						R2_CHECK(false, "Failed to generate texture pack manifest file from json!");
						return;
					}
				}

				if (!GenerateFromDirFunc(binaryPath.string(), rawPath.string(), materialPackDirBin, materialPackDirRaw))
				{
					R2_CHECK(false, "Failed to generate texture pack manifest file from directories!");
					return;
				}

			}
		}
	}

	s64 MaterialHotReloadCommand::FindIndexForPath(const std::string& path, std::string& nameOfMaterial)
	{
		std::filesystem::path materialPath = path;

		size_t index = 0;
		bool found = false;
		for (const std::string& rawWatchDir : mMaterialPacksWatchDirectoriesRaw)
		{
			std::filesystem::path rawWatchDirPath = rawWatchDir;
			std::filesystem::path changedPathParent = materialPath.parent_path().parent_path();
			nameOfMaterial = materialPath.parent_path().stem().string();

			while (!found && !changedPathParent.empty() && materialPath.root_path() != changedPathParent)
			{
				if (rawWatchDirPath == changedPathParent)
				{
					found = true;
				}
				else
				{
					nameOfMaterial = changedPathParent.stem().string();
					changedPathParent = changedPathParent.parent_path();
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

	void MaterialHotReloadCommand::MaterialChangedRequest(const std::string& changedPath)
	{
		std::filesystem::path changedMaterialPath = changedPath;

		R2_CHECK(changedMaterialPath.extension() == ".json", "This should be a json file!");
		std::string nameOfMaterial = "";
		s64 index = FindIndexForPath(changedPath, nameOfMaterial);

		if (index < 0)
		{
			return;
		}


		//we know which index to use now
		std::string manifestPathBin = mManifestBinaryFilePaths[index];
		std::string manifestPathRaw = mManifestRawFilePaths[index];
		std::string materialPackDirRaw = mMaterialPacksWatchDirectoriesRaw[index];
		std::string materialPackDirBin = mMaterialPacksWatchDirectoriesBin[index];


		//delete the mprm associated with this material
	    std::filesystem::path mtrlParentDirBinPath = std::filesystem::path(materialPackDirBin) / changedMaterialPath.parent_path().stem();
		

		for (const auto& file : std::filesystem::directory_iterator(mtrlParentDirBinPath))
		{
			if (file.path().extension() == MTRL_EXT)//MPRM_EXT)
			{
				std::filesystem::remove(file.path());
				break;
			}
		}

		//Then make the mprm again
		//GenerateMaterialParamsFromJSON(mprmParentDirBinPath.string(), changedPath);
		GenerateMaterialFromJSON(mtrlParentDirBinPath.string(), changedPath);

		//now regenerate the params packs manifest
		bool result = RegenerateMaterialPackManifest(manifestPathBin, manifestPathRaw, materialPackDirBin, materialPackDirRaw);

		if (!result)
		{
			R2_CHECK(false, "We failed to regenerate the material params pack manifest");
			return;
		}

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::asset::lib::PathChangedInManifest(assetLib, manifestPathBin, { changedPath });
	}

	void MaterialHotReloadCommand::MaterialAddedRequest(const std::string& newPath)
	{
		std::filesystem::path newMaterialPath = newPath;

		std::string nameOfMaterial = "";
		s64 index = FindIndexForPath(newPath, nameOfMaterial);

		if (index < 0)
		{
			return;
		}

		std::string manifestPathBin = mManifestBinaryFilePaths[index];
		std::string manifestPathRaw = mManifestRawFilePaths[index];
		std::string materialPackDirRaw = mMaterialPacksWatchDirectoriesRaw[index];
		std::string materialPackDirBin = mMaterialPacksWatchDirectoriesBin[index];

		std::filesystem::path mtrlParentDirBinPath = std::filesystem::path(materialPackDirBin) / nameOfMaterial;

		if (!std::filesystem::exists(mtrlParentDirBinPath))
		{
			std::filesystem::create_directory(mtrlParentDirBinPath);
		}

		//GenerateMaterialParamsFromJSON(mprmParentDirBinPath.string(), newPath);
		GenerateMaterialFromJSON(mtrlParentDirBinPath.string(), newPath);

		bool result = RegenerateMaterialPackManifest(manifestPathBin, manifestPathRaw, materialPackDirBin, materialPackDirRaw);

		if (!result)
		{
			R2_CHECK(false, "We failed to regenerate the material params pack manifest");
			return;
		}

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::asset::lib::PathAddedInManifest(assetLib, manifestPathBin, { newPath });
	}

	void MaterialHotReloadCommand::MaterialRemovedRequest(const std::string& removedPath)
	{
		std::filesystem::path removedMaterialPath = removedPath;

		std::string nameOfMaterial = "";
		s64 index = FindIndexForPath(removedPath, nameOfMaterial);

		if (index < 0)
		{
			return;
		}

		std::string manifestPathBin = mManifestBinaryFilePaths[index];
		std::string manifestPathRaw = mManifestRawFilePaths[index];
		std::string materialPackDirRaw = mMaterialPacksWatchDirectoriesRaw[index];
		std::string materialPackDirBin = mMaterialPacksWatchDirectoriesBin[index];

		std::filesystem::path mprmParentDirBinPath = std::filesystem::path(materialPackDirBin) / nameOfMaterial;

		for (const auto& file : std::filesystem::directory_iterator(mprmParentDirBinPath))
		{
			if (file.path().extension() == MTRL_EXT)
			{
				std::filesystem::remove(file.path());
				break;
			}
		}

		if (std::filesystem::exists(mprmParentDirBinPath))
		{
			std::filesystem::remove(mprmParentDirBinPath);
		}

		bool result = RegenerateMaterialPackManifest(manifestPathBin, manifestPathRaw, materialPackDirBin, materialPackDirRaw);

		if (!result)
		{
			R2_CHECK(false, "We failed to regenerate the material params pack manifest");
			return;
		}
		
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::asset::lib::PathRemovedInManifest(assetLib, manifestPathBin, { removedPath });
	}

}

#endif