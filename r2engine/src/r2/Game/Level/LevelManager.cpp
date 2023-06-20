
#include "r2pch.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Model/Materials/Material.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Game/ECSWorld/ECSWorld.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#endif // R2_ASSET_PIPELINE

#include "r2/Core/Engine.h"
#include "r2/Game/GameAssetManager/GameAssetManager.h"

namespace
{
	constexpr u32 AVG_LEVEL_SIZE = Kilobytes(10);
}

namespace r2
{
	const u32 LevelManager::MODEL_SYSTEM_SIZE = Megabytes(32);
	const u32 LevelManager::ANIMATION_CACHE_SIZE = Megabytes(8);
	const u32 LevelManager::MAX_NUM_MODELS = 100;
	const u32 LevelManager::MAX_NUM_ANIMATIONS = 500;

	LevelManager::LevelManager()
		:mMemoryAreaHandle(r2::mem::MemoryArea::Invalid)
		,mSubAreaHandle(r2::mem::MemoryArea::SubArea::Invalid)
		,mMaxNumLevels(0)
		,mArena(nullptr)
		,mLevelArena(nullptr)
		,mLoadedLevels(nullptr)
		,mSceneGraph{}
	{
	}

	LevelManager::~LevelManager()
	{
	}

	u64 LevelManager::GetSubAreaSizeForLevelManager(u32 numLevels, u32 numModels, u32 numAnimations, const r2::mem::utils::MemoryProperties& memProperties) const
	{
		u64 subAreaSize = 0;
		subAreaSize += MemorySize(numLevels, numModels, numAnimations, memProperties);

		return subAreaSize;
	}

	bool LevelManager::Init(r2::mem::MemoryArea::Handle memoryAreaHandle, ecs::ECSCoordinator* ecsCoordinator, const char* levelPackPath, const char* areaName, u32 maxNumLevels)
	{
		R2_CHECK(levelPackPath != nullptr, "We should have a proper path");
		R2_CHECK(strlen(levelPackPath) > 0, "We should have path that's more than 0");
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "We need a valid memory area");

		r2::mem::utils::MemoryProperties memProperties;
		memProperties.alignment = 16;

#ifdef R2_DEBUG
		memProperties.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		memProperties.headerSize = r2::mem::StackAllocator::HeaderSize();

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = GetSubAreaSizeForLevelManager(maxNumLevels, MAX_NUM_MODELS, MAX_NUM_ANIMATIONS, memProperties);

		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the LevelManager! We have: %llu bytes left but trying to allocate: %llu bytes, difference: %llu",
				memoryArea->UnAllocatedSpace(), subAreaSize, subAreaSize - memoryArea->UnAllocatedSpace());
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

		u32 numPoolElements = mMaxNumLevels;
		mLevelArena = MAKE_POOL_ARENA(*mArena, sizeof(Level), alignof(Level), numPoolElements);

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
		Level* theLevel = FindLevel(levelName, index);

		if (theLevel != nullptr)
		{
			return theLevel;
		}

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		LevelHandle levelHandle = gameAssetManager.LoadAsset(r2::asset::Asset(levelName, r2::asset::LEVEL));

		const byte* levelData = gameAssetManager.GetAssetDataConst<byte>(levelHandle);

		const flat::LevelData* flatLevelData = flat::GetLevelData(levelData);

		//now add the files to the file lists
		//these need to be before the scene graph load level
		AddModelFilesToModelSystem(flatLevelData);
		AddAnimationFilesToAnimationCache(flatLevelData);


		Level newLevel;
		newLevel.Init(flatLevelData, levelHandle);

		mSceneGraph.LoadedNewLevel(newLevel);

		r2::sarr::Push(*mLoadedLevels, newLevel);

		return r2::sarr::End(*mLoadedLevels);
	}

	const Level* LevelManager::GetLevel(const char* levelURI)
	{
		return GetLevel(STRING_ID(levelURI));
	}

	const Level* LevelManager::GetLevel(LevelName levelName)
	{
		s32 index = -1;
		return FindLevel(levelName, index);
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
		char fileURI[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::CopyFileNameWithParentDirectories(level->GetLevelData()->path()->str().c_str(), fileURI, 1);

		const auto levelHashName = STRING_ID(fileURI);

		Level defaultLevel = {};
		s32 index = -1;
		Level* theLevel = FindLevel(levelHashName, index);

		if (theLevel == nullptr)
		{
			//not even loaded - return
			return;
		}

		

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		gameAssetManager.UnloadAsset(theLevel->GetLevelHandle());

		r2::sarr::RemoveAndSwapWithLastElement(*mLoadedLevels, index);

	}

	bool LevelManager::IsLevelLoaded(LevelName levelName)
	{
		s32 index = -1;
		return FindLevel(levelName, index) != nullptr;
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

	void LevelManager::AddModelFilesToModelSystem(const flat::LevelData* levelData)
	{
		const auto numModels = levelData->modelFilePaths()->size();

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		const r2::asset::FileList fileList = gameAssetManager.GetFileList();

		for (flatbuffers::uoffset_t i = 0; i < numModels; ++i)
		{
			r2::asset::Asset animationAsset = r2::asset::Asset::MakeAssetFromFilePath(levelData->animationFilePaths()->Get(i)->binPath()->c_str(), r2::asset::RMODEL);

			if (gameAssetManager.HasAsset(animationAsset))
			{
				continue;
			}

			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(levelData->modelFilePaths()->Get(i)->binPath()->c_str());

			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
		}
	}

	Level* LevelManager::FindLevel(LevelName levelname, s32& index)
	{
		const u32 numLevels = r2::sarr::Size(*mLoadedLevels);

		for (u32 i = 0; i < numLevels; ++i)
		{
			Level& level = r2::sarr::At(*mLoadedLevels, i);
			if (level.GetLevelHashName() == levelname)
			{
				return &level;
			}
		}

		return nullptr;
	}

	void LevelManager::AddAnimationFilesToAnimationCache(const flat::LevelData* levelData)
	{
		const auto numAnimations = levelData->animationFilePaths()->size();

		r2::GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();
		
		const r2::asset::FileList fileList = gameAssetManager.GetFileList();
		
		for (flatbuffers::uoffset_t i = 0; i < numAnimations; ++i)
		{
			r2::asset::Asset animationAsset = r2::asset::Asset::MakeAssetFromFilePath(levelData->animationFilePaths()->Get(i)->binPath()->c_str(), r2::asset::RANIMATION);

			if (gameAssetManager.HasAsset(animationAsset))
			{
				continue;
			}

			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(levelData->animationFilePaths()->Get(i)->binPath()->c_str());

			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
		}
	}

	u64 LevelManager::MemorySize(
		u32 maxNumLevels,
		u32 maxNumModels,
		u32 maxNumAnimations,
		const r2::mem::utils::MemoryProperties& memProperties)
	{
		u64 poolHeaderSize = r2::mem::PoolAllocator::HeaderSize();
		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u64 memorySize = 0;

		auto stackArenaSize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		
		memorySize += stackHeaderSize;

		u64 poolSize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		poolSize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), alignof(Level), stackHeaderSize, memProperties.boundsChecking) * maxNumLevels;

		memorySize += poolSize;

		r2::mem::utils::MemoryProperties lvlCacheMemProps;
		lvlCacheMemProps.alignment = memProperties.alignment;
		lvlCacheMemProps.headerSize = stackHeaderSize;
		lvlCacheMemProps.boundsChecking = memProperties.boundsChecking;

		auto hashMapSize = r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Level>::MemorySize(maxNumLevels * r2::SHashMap<LevelName>::LoadFactorMultiplier()), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		
		memorySize += hashMapSize;
		
		auto sceneGraphMemory = SceneGraph::MemorySize(lvlCacheMemProps);

		memorySize += sceneGraphMemory;

		return memorySize;
	}

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
	void LevelManager::SaveNewLevelFile(
		u32 version,
		const char* binLevelPath,
		const char* rawJSONPath,
		const std::vector<r2::asset::AssetFile*>& modelFiles,
		const std::vector<r2::asset::AssetFile*>& animationFiles)
	{
		char sanitizedBinLevelPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(binLevelPath, sanitizedBinLevelPath);

		char sanitizedRawLevelPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(rawJSONPath, sanitizedRawLevelPath);

		r2::asset::Asset newLevelAsset = r2::asset::Asset::MakeAssetFromFilePath(sanitizedBinLevelPath, r2::asset::LEVEL);

		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		//first check to see if we have asset for this
		if (!gameAssetManager.HasAsset(newLevelAsset))
		{	
			const r2::asset::FileList fileList = gameAssetManager.GetFileList();
			
			//make a new asset file for the asset cache
			r2::asset::RawAssetFile* newFile = r2::asset::lib::MakeRawAssetFile(sanitizedBinLevelPath, r2::asset::GetNumberOfParentDirectoriesToIncludeForAssetType(r2::asset::LEVEL));

			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)newFile);
		}
		
		r2::ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();
		
		//Write out the new level file
		bool saved = r2::asset::pln::SaveLevelData(
			ecsWorld.GetECSCoordinator(),
			version,
			sanitizedBinLevelPath,
			sanitizedRawLevelPath,
			modelFiles, animationFiles);

		R2_CHECK(saved, "We couldn't save the file: %s\n", sanitizedBinLevelPath);
	}
#endif
}