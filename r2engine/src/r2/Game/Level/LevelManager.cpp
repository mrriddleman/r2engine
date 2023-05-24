
#include "r2pch.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Game/Level/LevelCache.h"
#include "r2/Render/Model/Materials/Material.h"
#include "r2/Render/Model/ModelCache.h"
#include "r2/Render/Animation/AnimationCache.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#include "r2/Core/Assets/AssetLib.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#endif // R2_ASSET_PIPELINE


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
		, mMaxNumLevels(0)
		,mArena(nullptr)
		,mLevelArena(nullptr)
		, mLoadedLevels(nullptr)
		,mLevelCache(nullptr)
		,mModelCache(nullptr)
		,mAnimationCache(nullptr)
		,mMaterialSystem(nullptr)
		, mSceneGraph{}
	{
	}

	LevelManager::~LevelManager()
	{
	}

	u64 LevelManager::GetSubAreaSizeForLevelManager(u32 numLevels, u32 numModels, u32 numAnimations, const r2::mem::utils::MemoryProperties& memProperties) const
	{
		u64 subAreaSize = 0;
		subAreaSize += MemorySize(numLevels, numModels, numAnimations, memProperties);
		subAreaSize -= r2::draw::modlche::MemorySize(numModels, MODEL_SYSTEM_SIZE);
		subAreaSize -= r2::draw::animcache::MemorySize(numAnimations, ANIMATION_CACHE_SIZE);

		return subAreaSize;
	}

	bool LevelManager::Init(r2::mem::MemoryArea::Handle memoryAreaHandle, ecs::ECSCoordinator* ecsCoordinator, GameAssetManager* noptrGameAssetManager, const char* levelPackPath, const char* areaName, u32 maxNumLevels)
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

		mLoadedLevels = MAKE_SHASHMAP(*mArena, Level, maxNumLevels * r2::SHashMap<Level>::LoadFactorMultiplier());

		mSceneGraph.Init<r2::mem::StackArena>(*mArena, ecsCoordinator);

		u32 levelCacheMemoryNeededInBytes = maxNumLevels * AVG_LEVEL_SIZE;

		u64 levelCacheMemorySizeNeededInBytes = lvlche::MemorySize(maxNumLevels, levelCacheMemoryNeededInBytes, memProperties);

		const auto unAllocated = mArena->UnallocatedBytes();
		if (!(unAllocated > levelCacheMemorySizeNeededInBytes))
		{
			R2_CHECK(false, "We don't have enough memory for the level cache. We have: %llu, but we need: %llu, difference: %llu\n",
				unAllocated, levelCacheMemorySizeNeededInBytes, levelCacheMemorySizeNeededInBytes - unAllocated);
			return false;
		}

		r2::mem::utils::MemBoundary levelCacheBoundary = MAKE_BOUNDARY(*mArena, levelCacheMemorySizeNeededInBytes, memProperties.alignment);

		mLevelCache = lvlche::CreateLevelCache(levelCacheBoundary, levelPackPath, mMaxNumLevels, levelCacheMemoryNeededInBytes);

		R2_CHECK(mLevelCache != nullptr, "We couldn't create the level cache correctly");

		r2::asset::FileList modelFiles = r2::asset::lib::MakeFileList(MAX_NUM_MODELS);

		mModelCache = r2::draw::modlche::Create(mMemoryAreaHandle, MODEL_SYSTEM_SIZE, true, modelFiles, "Level Manager Model System");

		r2::asset::FileList animationFiles = r2::asset::lib::MakeFileList(MAX_NUM_ANIMATIONS);

		mAnimationCache = r2::draw::animcache::Create(memoryAreaHandle, ANIMATION_CACHE_SIZE, animationFiles, "Level Manager Animation Cache");

		mnoptrGameAssetManager = noptrGameAssetManager;

		return true;
	}

	void LevelManager::Shutdown()
	{
		r2::draw::animcache::Shutdown(*mAnimationCache);

		r2::draw::modlche::Shutdown(mModelCache);

		auto iter = r2::shashmap::Begin(*mLoadedLevels);

		for (; iter != r2::shashmap::End(*mLoadedLevels); ++iter)
		{
			UnloadLevel(&iter->value);
		}

		r2::shashmap::Clear(*mLoadedLevels);

		mem::utils::MemBoundary levelCacheBoundary = mLevelCache->mLevelCacheBoundary;

		lvlche::Shutdown(mLevelCache);

		FREE(levelCacheBoundary.location, *mArena);

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
		if (!mLevelCache || !mLoadedLevels)
		{
			R2_CHECK(false, "We haven't initialized the LevelManager yet");
			return nullptr;
		}

		Level defaultLevel = {};

		Level& theLevel = r2::shashmap::Get(*mLoadedLevels, levelName, defaultLevel);

		if (!r2::asset::AreAssetHandlesEqual(theLevel.GetLevelHandle(), defaultLevel.GetLevelHandle()))
		{
			return &theLevel;
		}

		LevelHandle levelHandle = lvlche::LoadLevelData(*mLevelCache, levelName );

		const flat::LevelData* flatLevelData = lvlche::GetLevelData(*mLevelCache, levelHandle);

		//now add the files to the file lists
		//these need to be before the scene graph load level
		AddModelFilesToModelSystem(flatLevelData);
		AddAnimationFilesToAnimationCache(flatLevelData);


		Level newLevel;
		newLevel.Init(flatLevelData, levelHandle);

		mSceneGraph.LoadedNewLevel(newLevel);

		r2::shashmap::Set(*mLoadedLevels, levelName, newLevel);

		return &r2::shashmap::Get(*mLoadedLevels, levelName, defaultLevel);
	}

	const Level* LevelManager::GetLevel(const char* levelURI)
	{
		return GetLevel(STRING_ID(levelURI));
	}

	const Level* LevelManager::GetLevel(LevelName levelName)
	{
		Level defaultLevel = {};

		Level& theLevel = r2::shashmap::Get(*mLoadedLevels, levelName, defaultLevel);

		if (!r2::asset::AreAssetHandlesEqual(theLevel.GetLevelHandle(), defaultLevel.GetLevelHandle()))
		{
			return &theLevel;
		}

		return nullptr;
	}

	void LevelManager::UnloadLevel(const Level* level)
	{
		if (!mLevelCache || !mLoadedLevels)
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
		Level& theLevel = r2::shashmap::Get(*mLoadedLevels, levelHashName, defaultLevel);

		if (r2::asset::AreAssetHandlesEqual(defaultLevel.GetLevelHandle(), theLevel.GetLevelHandle()))
		{
			//not even loaded - return
			return;
		}

		r2::shashmap::Remove(*mLoadedLevels, levelHashName);

		lvlche::UnloadLevelData(*mLevelCache, theLevel.GetLevelHandle());
	}

	bool LevelManager::IsLevelLoaded(LevelName levelName)
	{
		Level defaultLevel;

		const Level& theLevel = r2::shashmap::Get(*mLoadedLevels, levelName, defaultLevel);

		return r2::asset::AreAssetHandlesEqual(defaultLevel.GetLevelHandle(), theLevel.GetLevelHandle());
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

	r2::draw::ModelCache* LevelManager::GetModelSystem()
	{
		return mModelCache;
	}

	r2::draw::AnimationCache* LevelManager::GetAnimationCache()
	{
		return mAnimationCache;
	}

	LevelName LevelManager::MakeLevelNameFromPath(const char* levelPath)
	{
		r2::asset::Asset levelAsset = r2::asset::Asset::MakeAssetFromFilePath(levelPath, r2::asset::LEVEL);
		return levelAsset.HashID();
	}

	void LevelManager::AddModelFilesToModelSystem(const flat::LevelData* levelData)
	{
		const auto numModels = levelData->modelFilePaths()->size();

		const r2::asset::FileList fileList = r2::draw::modlche::GetFileList(*mModelCache);

		for (flatbuffers::uoffset_t i = 0; i < numModels; ++i)
		{
			r2::asset::RawAssetFile* assetFile = r2::asset::lib::MakeRawAssetFile(levelData->modelFilePaths()->Get(i)->binPath()->c_str());

			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)assetFile);
		}
	}

	void LevelManager::AddAnimationFilesToAnimationCache(const flat::LevelData* levelData)
	{
		const auto numAnimations = levelData->animationFilePaths()->size();

		const r2::asset::FileList fileList = r2::draw::animcache::GetFileList(*mAnimationCache);

		for (flatbuffers::uoffset_t i = 0; i < numAnimations; ++i)
		{
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

		u64 levelCacheSize = lvlche::MemorySize(maxNumLevels, maxNumLevels * AVG_LEVEL_SIZE, lvlCacheMemProps);

		memorySize += levelCacheSize;

		auto hashMapSize = r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Level>::MemorySize(maxNumLevels * r2::SHashMap<LevelName>::LoadFactorMultiplier()), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		
		memorySize += hashMapSize;
		
		auto sceneGraphMemory = SceneGraph::MemorySize(lvlCacheMemProps);

		memorySize += sceneGraphMemory;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::modlche::MemorySize(maxNumModels, MODEL_SYSTEM_SIZE), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::animcache::MemorySize(maxNumAnimations, ANIMATION_CACHE_SIZE), memProperties.alignment, stackHeaderSize, memProperties.boundsChecking);

		
		return memorySize;
	}

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
	void LevelManager::SaveNewLevelFile(
		u32 version,
		const char* binLevelPath,
		const char* rawJSONPath,
		const r2::draw::ModelCache& modelSystem,
		const r2::draw::AnimationCache& animationCache)
	{
		if (!mLevelCache)
		{
			R2_CHECK(false, "We haven't created the level cache yet!");

			return;
		}

		lvlche::SaveNewLevelFile(
			*mLevelCache,
			mSceneGraph.GetECSCoordinator(),
			version,
			binLevelPath,
			rawJSONPath,
			r2::draw::modlche::GetFileList(modelSystem),
			r2::draw::animcache::GetFileList(animationCache));
	}
#endif
}