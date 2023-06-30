
#include "r2pch.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Game/ECSWorld/ECSWorld.h"
#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"

#endif // R2_ASSET_PIPELINE

#ifdef R2_EDITOR
#include "r2/Editor/EditorLevel.h"
#endif

#include "r2/Core/Engine.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"

namespace r2
{
	const u32 LevelManager::MAX_NUM_MODELS = 100;
	const u32 LevelManager::MAX_NUM_ANIMATIONS = 500;
	const u32 LevelManager::MAX_NUM_TEXTURE_PACKS = 100;
	
	LevelManager::LevelManager()
		:mMemoryAreaHandle(r2::mem::MemoryArea::Invalid)
		,mSubAreaHandle(r2::mem::MemoryArea::SubArea::Invalid)
		,mMaxNumLevels(0)
		,mArena(nullptr)
		,mLoadedLevels(nullptr)
		,mSceneGraph{}
	{
	}

	LevelManager::~LevelManager()
	{
	}

	bool LevelManager::Init(
		r2::mem::MemoryArea::Handle memoryAreaHandle,
		ecs::ECSCoordinator* ecsCoordinator,
		const char* levelPackPath,
		const char* areaName,
		u32 maxNumLevels,
		const char* binLevelOutputPath,
		const char* rawLevelOutputPath)
	{
		R2_CHECK(levelPackPath != nullptr, "We should have a proper path");
		R2_CHECK(strlen(levelPackPath) > 0, "We should have path that's more than 0");
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "We need a valid memory area");

		R2_CHECK(binLevelOutputPath != nullptr && strlen(binLevelOutputPath) > 0, "We don't have a proper path");
		R2_CHECK(rawLevelOutputPath != nullptr && strlen(rawLevelOutputPath) > 0, "We don't have a proper path");

		r2::fs::utils::SanitizeSubPath(binLevelOutputPath, mBinOutputPath);
		r2::fs::utils::SanitizeSubPath(rawLevelOutputPath, mRawOutputPath);

		r2::mem::utils::MemoryProperties memProperties;
		memProperties.alignment = 16;

#ifdef R2_DEBUG
		memProperties.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		memProperties.headerSize = r2::mem::StackAllocator::HeaderSize();

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(maxNumLevels, MAX_NUM_MODELS, MAX_NUM_ANIMATIONS, MAX_NUM_TEXTURE_PACKS, ecs::MAX_NUM_ENTITIES, memProperties);
		u64 unallocated = memoryArea->UnAllocatedSpace();
		if (unallocated < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the LevelManager! We have: %llu bytes left but trying to allocate: %llu bytes, difference: %llu",
				unallocated, subAreaSize, subAreaSize - unallocated);
			return false;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		if ((subAreaHandle = memoryArea->AddSubArea(subAreaSize, areaName)) == r2::mem::MemoryArea::SubArea::Invalid)
		{
			R2_CHECK(false, "We couldn't create a sub area for %s", areaName);
			return false;
		}

		mArena = EMPLACE_STACK_ARENA(*memoryArea->GetSubArea(subAreaHandle));

		R2_CHECK(mArena != nullptr, "We couldn't emplace the stack arena!");

		mMaxNumLevels = maxNumLevels;

		//Make free list arena
		u32 freeListArenaSize = 0;
		freeListArenaSize += (r2::SArray<r2::asset::AssetHandle>::MemorySize(MAX_NUM_MODELS) + r2::SArray<r2::asset::AssetHandle>::MemorySize(MAX_NUM_ANIMATIONS))* maxNumLevels;
		freeListArenaSize += r2::SArray<u64>::MemorySize(MAX_NUM_TEXTURE_PACKS) * maxNumLevels;

		mLevelArena = MAKE_FREELIST_ARENA(*mArena, freeListArenaSize, r2::mem::FIND_BEST);
		R2_CHECK(mLevelArena != nullptr, "We couldn't make the level arena");

		mMemoryAreaHandle = memoryAreaHandle;
		mSubAreaHandle = subAreaHandle;

		mLoadedLevels = MAKE_SARRAY(*mArena, Level, maxNumLevels);

		mSceneGraph.Init<r2::mem::StackArena>(*mArena, ecsCoordinator);


		//@TODO(Serge): add in the level files from the path
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
		r2::asset::FileList fileList = gameAssetManager.GetFileList();

		//@Temporary
		for (auto& file : std::filesystem::recursive_directory_iterator(levelPackPath))
		{
			if (!(file.is_regular_file() && file.file_size() > 0))
			{
				continue;
			}

			r2::asset::RawAssetFile* levelFile = r2::asset::lib::MakeRawAssetFile(file.path().string().c_str(), r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::LEVEL));
			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)levelFile);
		}

		return true;
	}

	void LevelManager::Shutdown()
	{
		u32 numLevels = r2::sarr::Size(*mLoadedLevels);

		for (u32 i = 0; i < numLevels; ++i)
		{
			Level& level = r2::sarr::At(*mLoadedLevels, i);
			UnloadLevel(&level);
		}

		r2::sarr::Clear(*mLoadedLevels);

		mSceneGraph.Shutdown<r2::mem::StackArena>(*mArena);

		FREE(mLoadedLevels, *mArena);

		FREE(mLevelArena, *mArena);

		FREE_EMPLACED_ARENA(mArena);

		//@TODO(Serge): should we have to remove the sub areas?
	}

	void LevelManager::Update()
	{
		mSceneGraph.Update();
	}

	const Level* LevelManager::MakeNewLevel(LevelName levelName)
	{
		Level newLevel;

		r2::SArray<r2::asset::AssetHandle>* modelAssets = MAKE_SARRAY(*mLevelArena, r2::asset::AssetHandle, MAX_NUM_MODELS);
		r2::SArray<r2::asset::AssetHandle>* animationAssets = MAKE_SARRAY(*mLevelArena, r2::asset::AssetHandle, MAX_NUM_ANIMATIONS);
		r2::SArray<u64>* texturePackAssets = MAKE_SARRAY(*mLevelArena, u64, MAX_NUM_TEXTURE_PACKS);
		r2::SArray<ecs::Entity>* entities = MAKE_SARRAY(*mLevelArena, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		//@NOTE(Serge): sort of weird we're doing this but it's only when we make new levels for the editor
		//				we may want to have this method only for R2_EDITOR/R2_ASSET_PIPELINE
		newLevel.Init(nullptr, {levelName, gameAssetManager.GetAssetCacheSlot()}, modelAssets, animationAssets, texturePackAssets, entities);

		r2::sarr::Push(*mLoadedLevels, newLevel);

		return &r2::sarr::Last(*mLoadedLevels);
	}

	const r2::Level* LevelManager::LoadLevel(const char* levelURI)
	{
		return LoadLevel(STRING_ID(levelURI));
	}

	const r2::Level* LevelManager::LoadLevel(LevelName levelName)
	{
		if (!mLoadedLevels)
		{
			R2_CHECK(false, "We haven't initialized the LevelManager yet");
			return nullptr;
		}

		Level defaultLevel = {};

		s32 index = -1;
		Level* theLevel = FindLoadedLevel(levelName, index);

		if (theLevel != nullptr)
		{
			return theLevel;
		}

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		LevelHandle levelHandle = gameAssetManager.LoadAsset(r2::asset::Asset(levelName, r2::asset::LEVEL));

		const byte* levelData = gameAssetManager.GetAssetDataConst<byte>(levelHandle);

		const flat::LevelData* flatLevelData = flat::GetLevelData(levelData);

		Level newLevel;

		r2::SArray<r2::asset::AssetHandle>* modelAssets = MAKE_SARRAY(*mLevelArena, r2::asset::AssetHandle, flatLevelData->modelFilePaths()->size());
		r2::SArray<r2::asset::AssetHandle>* animationAssets = MAKE_SARRAY(*mLevelArena, r2::asset::AssetHandle, flatLevelData->animationFilePaths()->size());
		r2::SArray<u64>* texturePackAssets = MAKE_SARRAY(*mLevelArena, u64, flatLevelData->materialNames()->size() * r2::draw::tex::Cubemap);
		r2::SArray<ecs::Entity>* entities = MAKE_SARRAY(*mLevelArena, ecs::Entity, ecs::MAX_NUM_ENTITIES);

		newLevel.Init(flatLevelData, levelHandle, modelAssets, animationAssets, texturePackAssets, entities);

		LoadLevelData(newLevel);

		mSceneGraph.LoadLevel(newLevel);

		r2::sarr::Push(*mLoadedLevels, newLevel);

		return &r2::sarr::Last(*mLoadedLevels);
	}

	void LevelManager::UnloadLevel(const Level* level)
	{
		if (!mLoadedLevels)
		{
			R2_CHECK(false, "We haven't initialized the LevelManager yet");
			return;
		}
		
		if (!level)
		{
			R2_CHECK(false, "Passed in nullptr for the level");
			return;
		}

		//make the hash name
		const auto levelHashName = level->GetLevelAssetName();

		Level defaultLevel = {};
		s32 index = -1;
		Level* theLevel = FindLoadedLevel(levelHashName, index);

		if (theLevel == nullptr)
		{
			//not even loaded - return
			return;
		}

		mSceneGraph.UnloadLevel(*theLevel);
		
		theLevel->ClearAllEntities();
		
		Level copyOfLevel = *theLevel;

		r2::sarr::RemoveAndSwapWithLastElement(*mLoadedLevels, index);

		UnLoadLevelData(copyOfLevel);

		FREE(copyOfLevel.mEntities, *mLevelArena);
		FREE(copyOfLevel.mTexturePackAssets, *mLevelArena);
		FREE(copyOfLevel.mAnimationAssets, *mLevelArena);
		FREE(copyOfLevel.mModelAssets, *mLevelArena);

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		gameAssetManager.UnloadAsset(copyOfLevel.GetLevelHandle());
		
		copyOfLevel.Shutdown();
	}

	const Level* LevelManager::GetLevel(const char* levelURI)
	{
		return GetLevel(STRING_ID(levelURI));
	}

	const Level* LevelManager::GetLevel(LevelName levelName)
	{
		s32 index = -1;
		return FindLoadedLevel(levelName, index);
	}

	bool LevelManager::IsLevelLoaded(LevelName levelName)
	{
		s32 index = -1;
		return FindLoadedLevel(levelName, index) != nullptr;
	}

	bool LevelManager::IsLevelLoaded(const char* levelName)
	{
		return IsLevelLoaded(STRING_ID(levelName));
	}

	SceneGraph& LevelManager::GetSceneGraph()
	{
		return mSceneGraph;
	}

	SceneGraph* LevelManager::GetSceneGraphPtr()
	{
		return &mSceneGraph;
	}

	LevelName LevelManager::MakeLevelNameFromPath(const char* levelPath)
	{
		r2::asset::Asset levelAsset = r2::asset::Asset::MakeAssetFromFilePath(levelPath, r2::asset::LEVEL);
		return levelAsset.HashID();
	}

	Level* LevelManager::FindLoadedLevel(LevelName levelname, s32& index)
	{
		const u32 numLevels = r2::sarr::Size(*mLoadedLevels);

		for (u32 i = 0; i < numLevels; ++i)
		{
			Level& level = r2::sarr::At(*mLoadedLevels, i);
			if (level.GetLevelAssetName() == levelname)
			{
				index = i;
				return &level;
			}
		}

		index = -1;
		return nullptr;
	}

	void LevelManager::LoadLevelData(Level& level)
	{
		const flat::LevelData* levelData = level.GetLevelData();
		r2::SArray<r2::asset::AssetHandle>* modelAssets = level.mModelAssets;
		r2::SArray<r2::asset::AssetHandle>* animationAssets = level.mAnimationAssets;
		r2::SArray<u64>* textureAssets = level.mTexturePackAssets;

		f64 start = CENG.GetTicks();

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		const auto* modelFiles = levelData->modelFilePaths();

		const flatbuffers::uoffset_t numModelFiles = modelFiles->size();

		f64 startModelLoading = start;

		for (flatbuffers::uoffset_t i = 0; i < numModelFiles; ++i)
		{
			r2::asset::Asset modelAsset = r2::asset::Asset::MakeAssetFromFilePath(modelFiles->Get(i)->binPath()->str().c_str(), r2::asset::RMODEL);
			r2::sarr::Push(*modelAssets, gameAssetManager.LoadAsset(modelAsset));

			//@NOTE(Serge): maybe we should be uploading the model to the GPU - however that would require us knowing 
			//				what kind of model it is. Maybe we need to make a universal model so this doesn't keep happening.
			//				Since each entity loads and uploads their models when hydrating, this isn't important at the moment.
		}

		f64 endModelLoading = CENG.GetTicks();
		
		f64 startAnimationLoading = endModelLoading;
		const auto* animationFiles = levelData->animationFilePaths();

		const flatbuffers::uoffset_t numAnimationFiles = animationFiles->size();
		 
		for (flatbuffers::uoffset_t i = 0; i < numAnimationFiles; ++i)
		{
			r2::asset::Asset animationAsset = r2::asset::Asset::MakeAssetFromFilePath(animationFiles->Get(i)->binPath()->str().c_str(), r2::asset::RANIMATION);
			
			r2::sarr::Push(*animationAssets, gameAssetManager.LoadAsset(animationAsset));
		}

		f64 endAnimationLoading = CENG.GetTicks();
		
		f64 startMaterialLoading = endAnimationLoading;


		f64 materialGatherStart;
		f64 materialGatherEnd;

		f64 materialDiskLoadStart;
		f64 materialDiskLoadEnd;

		f64 materialUploadStart;
		f64 materialUploadEnd;

		f64 totalTextureUploadTime = 0;
		//load the materials
		//@TODO(Serge): probably all of this will change when we do the texture refactor
		{

			materialGatherStart = CENG.GetTicks();
			const auto* materialNames = levelData->materialNames();

			const flatbuffers::uoffset_t numMaterials = materialNames->size();

			r2::SArray<const flat::MaterialParams*>* materialParamsToLoad = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, const flat::MaterialParams*, numMaterials);

			u32 numTexturesTotal = 0;
			u32 numCubemapTexturesTotal = numMaterials;

			//gather all the materials
			for (flatbuffers::uoffset_t i = 0; i < numMaterials; ++i)
			{
				const flat::MaterialName* flatMaterialName = materialNames->Get(i);

				r2::mat::MaterialName materialName = r2::mat::MakeMaterialNameFromFlatMaterial(flatMaterialName);

				const flat::MaterialParams* materialParams = r2::mat::GetMaterialParamsForMaterialName(materialName);

				R2_CHECK(materialParams != nullptr, "This should never be nullptr");

				numTexturesTotal = std::max(materialParams->textureParams()->size(), numTexturesTotal);

				r2::sarr::Push(*materialParamsToLoad, materialParams);

				r2::mat::GetAllTexturePacksForMaterial(materialParams, textureAssets);
			}

			materialGatherEnd = CENG.GetTicks();
			materialDiskLoadStart = materialGatherEnd;
			//Load from disk
			for (u32 i = 0; i < numMaterials; ++i)
			{
				bool result = gameAssetManager.LoadMaterialTextures(r2::sarr::At(*materialParamsToLoad, i));
				R2_CHECK(result, "Should always work");
			}

			materialDiskLoadEnd = CENG.GetTicks();
			materialUploadStart = materialDiskLoadEnd;

			//load to gpu
			r2::SArray<r2::draw::tex::Texture>* gameTextures = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::Texture, numTexturesTotal);
			r2::SArray<r2::draw::tex::CubemapTexture>* gameCubemaps = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::tex::CubemapTexture, numTexturesTotal);

			r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();
			for (u32 i = 0; i < numMaterials; ++i)
			{

				const flat::MaterialParams* materialParams = r2::sarr::At(*materialParamsToLoad, i);
				bool result = gameAssetManager.GetTexturesForMaterialParams(materialParams, gameTextures, gameCubemaps);
				R2_CHECK(result, "Should always work");


				r2::SArray<r2::draw::tex::Texture>* texturesToUse = nullptr;
				if (r2::sarr::Size(*gameTextures) > 0)
				{
					texturesToUse = gameTextures;
				}

				r2::draw::tex::CubemapTexture* cubemapTexture = nullptr;
				if (r2::sarr::Size(*gameCubemaps) > 0)
				{
					cubemapTexture = &r2::sarr::At(*gameCubemaps, 0);
				}

				f64 texParamUploadStart = CENG.GetTicks();
				r2::draw::rmat::UploadMaterialTextureParams(*renderMaterialCache, materialParams, texturesToUse, cubemapTexture, false);
				f64 texParamUploadEnd = CENG.GetTicks();
				totalTextureUploadTime += (texParamUploadEnd - texParamUploadStart);

				r2::sarr::Clear(*gameTextures);
				r2::sarr::Clear(*gameCubemaps);
			}

			FREE(gameCubemaps, *MEM_ENG_SCRATCH_PTR);
			FREE(gameTextures, *MEM_ENG_SCRATCH_PTR);

			FREE(materialParamsToLoad, *MEM_ENG_SCRATCH_PTR);

			materialUploadEnd = CENG.GetTicks();
		}

		f64 end = materialUploadEnd;

		//@TODO(Serge): Load the sounds + music
		{
			//const auto* soundFiles = levelData->soundPaths();

			//const flatbuffers::uoffset_t numSoundFiles = soundFiles->size();

			//for (flatbuffers::uoffset_t i = 0; i < numSoundFiles; ++i)
			//{
			//	//@TODO(Serge): implement
			//}
		}
		
#ifdef R2_DEBUG
		printf("Model loading took: %f\n", endModelLoading - startModelLoading);
		printf("Animation loading took: %f\n", endAnimationLoading - startAnimationLoading);
		printf("Material Gather took: %f\n", materialGatherEnd - materialGatherStart);
		printf("Material disk load took: %f\n", materialDiskLoadEnd - materialDiskLoadStart);
		printf("Material upload took: %f\n", materialUploadEnd - materialUploadStart);
		printf("Material loading took: %f\n", end - startMaterialLoading);
		printf("Texture param upload took: %f\n", totalTextureUploadTime);
		printf("Total load time: %f\n", end - start);
#endif
	}

	void LevelManager::UnLoadLevelData(const Level& level)
	{
		//I think the idea is to figure out which models/animations/materials/sounds etc to unload 
		//based on what other levels are already loaded, that way we don't unload anything that we still need
		//since we know levels will essentially be based on different kinds of packs (sounds, textures, models, animations etc).
		
		//build the lists of assets we need to unload

		//@NOTE(Serge): we shouldn't have the level in the mLoadedLevel list at this point

		r2::SArray<r2::asset::AssetHandle>* modelAssetsToUnload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::AssetHandle, r2::sarr::Size(*level.mModelAssets));
		r2::SArray<r2::asset::AssetHandle>* animationAssetsToUnload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::asset::AssetHandle, r2::sarr::Size(*level.mAnimationAssets));
		r2::SArray<u64>* texturePacksToUnload = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, r2::sarr::Size(*level.mTexturePackAssets));

		const u32 numLoadedLevels = r2::sarr::Size(*mLoadedLevels);
		const u32 numModelsInLevel = r2::sarr::Size(*level.mModelAssets);
		const u32 numAnimationsInLevel = r2::sarr::Size(*level.mAnimationAssets);
		const u32 numTexturePacksInLevel = r2::sarr::Size(*level.mTexturePackAssets);

		for (u32 j = 0; j < numModelsInLevel; ++j)
		{
			auto modelAsset = r2::sarr::At(*level.mModelAssets, j);

			bool found = false;

			for (u32 i = 0; i < numLoadedLevels && !found; ++i)
			{
				const Level& loadedLevel = r2::sarr::At(*mLoadedLevels, i);
				//now go through each model and see if it's in our level, if it isn't in there then add to the list
				if (r2::sarr::IndexOf(*loadedLevel.mModelAssets, modelAsset) != -1)
				{
					found = true;
				}
			}

			if (!found)
			{
				r2::sarr::Push(*modelAssetsToUnload, modelAsset);
			}
		}

		for (u32 j = 0; j < numAnimationsInLevel; ++j)
		{
			auto animationAsset = r2::sarr::At(*level.mAnimationAssets, j);

			bool found = false;

			for (u32 i = 0; i < numLoadedLevels && !found; ++i)
			{
				const Level& loadedLevel = r2::sarr::At(*mLoadedLevels, i);
				//now go through each model and see if it's in our level, if it isn't in there then add to the list
				if (r2::sarr::IndexOf(*loadedLevel.mAnimationAssets, animationAsset) != -1)
				{
					found = true;
				}
			}

			if (!found)
			{
				r2::sarr::Push(*animationAssetsToUnload, animationAsset);
			}
		}

		for (u32 j = 0; j < numTexturePacksInLevel; ++j)
		{
			auto texturePackAsset = r2::sarr::At(*level.mTexturePackAssets, j);

			bool found = false;

			for (u32 i = 0; i < numLoadedLevels && !found; ++i)
			{
				const Level& loadedLevel = r2::sarr::At(*mLoadedLevels, i);
				//now go through each model and see if it's in our level, if it isn't in there then add to the list
				if (r2::sarr::IndexOf(*loadedLevel.mTexturePackAssets, texturePackAsset) != -1)
				{
					found = true;
				}
			}

			if (!found)
			{
				r2::sarr::Push(*texturePacksToUnload, texturePackAsset);
			}
		}

		//now we have the assets to unload
		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		const u32 numModelsToUnload = r2::sarr::Size(*modelAssetsToUnload);
		for (u32 i = 0; i < numModelsToUnload; ++i)
		{
			const r2::asset::AssetHandle& assetHandle = r2::sarr::At(*modelAssetsToUnload, i);

			r2::draw::renderer::UnloadModel(r2::draw::renderer::GetModelRefHandleForModelAssetName(assetHandle.handle));

			gameAssetManager.UnloadAsset(assetHandle);
		}
		
		const u32 numAnimationsToUnload = r2::sarr::Size(*animationAssetsToUnload);
		for (u32 i = 0; i < numAnimationsToUnload; ++i)
		{
			gameAssetManager.UnloadAsset(r2::sarr::At(*animationAssetsToUnload, i));
		}

		const u32 numTexturePacksToUnload = r2::sarr::Size(*texturePacksToUnload);
		for (u32 i = 0; i < numTexturePacksToUnload; ++i)
		{
			gameAssetManager.UnloadTexturePack(r2::sarr::At(*texturePacksToUnload, i));
		}

		FREE(texturePacksToUnload, *MEM_ENG_SCRATCH_PTR);
		FREE(animationAssetsToUnload, *MEM_ENG_SCRATCH_PTR);
		FREE(modelAssetsToUnload, *MEM_ENG_SCRATCH_PTR);
	}

	u64 LevelManager::MemorySize(
		u32 maxNumLevels,
		u32 maxNumModels,
		u32 maxNumAnimations,
		u32 maxNumTexturePacks,
		u32 maxNumEntities,
		const r2::mem::utils::MemoryProperties& memProperties)
	{
		u64 freeListHeaderSize = r2::mem::FreeListAllocator::HeaderSize();
		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u64 memorySize = 0;

		auto stackArenaSize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		
		memorySize += stackHeaderSize;

		r2::mem::utils::MemoryProperties lvlArenaMemProps;
		lvlArenaMemProps.alignment = memProperties.alignment;
		lvlArenaMemProps.headerSize = freeListHeaderSize;
		lvlArenaMemProps.boundsChecking = memProperties.boundsChecking;

		u64 freeListArenaSize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::FreeListArena), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);

		freeListArenaSize += Level::MemorySize(maxNumModels, maxNumAnimations, maxNumTexturePacks, maxNumEntities, lvlArenaMemProps) * maxNumLevels;
		
		memorySize += freeListArenaSize;
		

		r2::mem::utils::MemoryProperties lvlCacheMemProps;
		lvlCacheMemProps.alignment = memProperties.alignment;
		lvlCacheMemProps.headerSize = stackHeaderSize;
		lvlCacheMemProps.boundsChecking = memProperties.boundsChecking;

		auto hashMapSize = r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Level>::MemorySize(maxNumLevels), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		
		memorySize += hashMapSize;
		
		auto sceneGraphMemory = SceneGraph::MemorySize(lvlCacheMemProps);

		memorySize += sceneGraphMemory;

		return memorySize;
	}

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
	void LevelManager::SaveNewLevelFile(const EditorLevel& editorLevel)
	{
		char binLevelPath[r2::fs::FILE_PATH_LENGTH];
		char rawLevelPath[r2::fs::FILE_PATH_LENGTH];

		std::string levelBinURI = editorLevel.GetGroupName() + r2::fs::utils::PATH_SEPARATOR + editorLevel.GetLevelName() + ".rlvl";
		std::string levelRawURI = editorLevel.GetGroupName() + r2::fs::utils::PATH_SEPARATOR + editorLevel.GetLevelName() + ".json";

		r2::fs::utils::AppendSubPath(mBinOutputPath, binLevelPath, levelBinURI.c_str());
		r2::fs::utils::AppendSubPath(mRawOutputPath, rawLevelPath, levelRawURI.c_str());

		r2::asset::Asset newLevelAsset = r2::asset::Asset::MakeAssetFromFilePath(binLevelPath, r2::asset::LEVEL);

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		//first check to see if we have asset for this
		if (!gameAssetManager.HasAsset(newLevelAsset))
		{	
			const r2::asset::FileList fileList = gameAssetManager.GetFileList();
			
			//make a new asset file for the asset cache
			r2::asset::RawAssetFile* newFile = r2::asset::lib::MakeRawAssetFile(binLevelPath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::LEVEL));

			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)newFile);
		}
		
		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();
		
		//Write out the new level file
		bool saved = r2::asset::pln::SaveLevelData(
			ecsWorld.GetECSCoordinator(), binLevelPath, rawLevelPath, editorLevel);

		R2_CHECK(saved, "We couldn't save the file: %s\n", binLevelPath);
	}
#endif
}