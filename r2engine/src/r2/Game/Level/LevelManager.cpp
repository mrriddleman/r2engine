
#include "r2pch.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Game/Level/LevelCache.h"
#include "r2/Render/Model/Material.h"
#include "r2/Render/Model/ModelSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Utils/Hash.h"
#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#endif // R2_ASSET_PIPELINE


namespace
{
	constexpr u32 AVG_LEVEL_SIZE = Kilobytes(10);
}

namespace r2
{
	LevelManager::LevelManager()
		:mMemoryAreaHandle(r2::mem::MemoryArea::Invalid)
		,mSubAreaHandle(r2::mem::MemoryArea::SubArea::Invalid)
		, mMaxNumLevels(0)
	//	, mMaxNumLevelsLoadedAtOneTime(0)
		,mArena(nullptr)
		,mLevelArena(nullptr)
		, mLoadedLevels(nullptr)
		,mLevelCache(nullptr)
		

	{
	}

	LevelManager::~LevelManager()
	{
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

		u64 subAreaSize = MemorySize(
			maxNumLevels,
			memProperties);

		if (memoryArea->UnAllocatedSpace() < subAreaSize)
		{
			R2_CHECK(false, "We don't have enough space to allocate the LevelManager! We have: %llu bytes left but trying to allocate: %llu bytes",
				memoryArea->UnAllocatedSpace(), subAreaSize);
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
		mLevelArena = MAKE_POOL_ARENA(*mArena, sizeof(Level), numPoolElements);

		mMemoryAreaHandle = memoryAreaHandle;
		mSubAreaHandle = subAreaHandle;

		mLoadedLevels = MAKE_SHASHMAP(*mArena, Level, maxNumLevels * r2::SHashMap<Level>::LoadFactorMultiplier());
		
		mSceneGraph.Init<r2::mem::StackArena>(*mArena, ecsCoordinator);

		//mMaxNumLevelsLoadedAtOneTime = numLevelsLoadedAtOneTime;

		u32 levelCacheMemoryNeededInBytes = maxNumLevels * AVG_LEVEL_SIZE;

		u32 levelCacheMemorySizeNeededInBytes = lvlche::MemorySize(maxNumLevels, levelCacheMemoryNeededInBytes, memProperties);

		if (!(mArena->UnallocatedBytes() > levelCacheMemorySizeNeededInBytes))
		{
			R2_CHECK(false, "We don't have enough memory for the level cache. We have: %llu, but we need: %llu, difference: %llu\n",
				mArena->UnallocatedBytes(), levelCacheMemorySizeNeededInBytes, levelCacheMemorySizeNeededInBytes - mArena->UnallocatedBytes());
			return false;
		}

		r2::mem::utils::MemBoundary levelCacheBoundary = MAKE_BOUNDARY(*mArena, levelCacheMemorySizeNeededInBytes, memProperties.alignment);

		mLevelCache = lvlche::CreateLevelCache(levelCacheBoundary, levelPackPath, mMaxNumLevels, levelCacheMemoryNeededInBytes);

		R2_CHECK(mLevelCache != nullptr, "We couldn't create the level cache correctly");


//		//We always have to have atleast 1 level group
//		if (numLevelGroupsLoadedAtOneTime == 0)
//		{
//			numLevelGroupsLoadedAtOneTime = 1;
//		}
//
//		if (numLevelsPerGroupLoadedAtOneTime == 0)
//		{
//			numLevelsPerGroupLoadedAtOneTime = 1;
//		}
//
//		u32 totalNumberOfLevels = 0;
//		u32 maxNumLevelsInGroup = 0;
//		u32 maxLevelSizeInBytes = 0;
//		u32 maxGroupSizeInBytes = 0;
//		u64 levelPackDataFileSize = 0;
//		u32 maxGroupFileSize = 0;
//		u32 levelCacheMemoryNeededInBytes = 0;
//		u32 numModelSystems = 0;
//		u32 numMaterialSystems = 0;
//		u32 numSoundDefinitionFiles = 0;
//
//		u32 maxNumMaterialsInALevel = 0;
//		u32 maxNumTexturesInAPackForALevel = 0;
//		u32 maxTextureSizeInAPackForALevel = 0;
//		u32 maxNumTexturePacksForAMaterialForALevel = 0;
//		u32 maxTotalNumberOfTextures = 0;
//		u32 maxTexturePackFileSize = 0;
//		u32 maxMaterialPackFileSize = 0;
//
//		u32 maxNumModelsInAPackInALevel = 0;
//		u32 maxModelPackSizeInALevel = 0;
//
//		void* levelPackDataFile = nullptr;
//		const flat::LevelPackData* levelPackData = nullptr;
//
//		if (r2::fs::FileSystem::FileExists(levelPackPath))
//		{
//			levelPackDataFile = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, levelPackPath, levelPackDataFileSize);
//
//			levelPackData = flat::GetLevelPackData(levelPackDataFile);
//
//			maxNumLevelsInGroup = levelPackData->maxLevelsInAGroup();
//			maxLevelSizeInBytes = levelPackData->maxLevelSizeInBytes();
//			maxGroupSizeInBytes = levelPackData->maxGroupSizeInBytes();
//			numModelSystems = levelPackData->maxNumAnimModelPackReferencesInALevel() + levelPackData->maxNumStaticModelPackReferencesInALevel();
//			numMaterialSystems = levelPackData->maxNumMaterialPackReferencesInALevel();
//			numSoundDefinitionFiles = levelPackData->maxNumSoundReferencesInALevel() + levelPackData->maxNumMusicReferencesInALevel();
//
//			maxNumMaterialsInALevel = levelPackData->maxNumMaterialsInALevel();
//			maxNumTexturesInAPackForALevel = levelPackData->maxNumTexturesInAPackForALevel();
//			maxTextureSizeInAPackForALevel = levelPackData->maxTextureSizeInAPackForALevel();
//			maxNumTexturePacksForAMaterialForALevel = levelPackData->maxNumTexturePacksForAMaterialForALevel();
//			maxTotalNumberOfTextures = levelPackData->maxTotalNumberOfTextures();
//			maxTexturePackFileSize = levelPackData->maxTexturePackFileSize();
//			maxMaterialPackFileSize = levelPackData->maxMaterialPackFileSize();
//
//			maxNumModelsInAPackInALevel = levelPackData->maxNumModelsInALevel();
//			maxModelPackSizeInALevel = levelPackData->maxModelPackSize();
//
//			//now figure out how many files to make
//			for (flatbuffers::uoffset_t i = 0; i < levelPackData->allLevels()->size(); ++i)
//			{
//				auto numLevelsInGroup = levelPackData->allLevels()->Get(i)->levels()->size();
//				totalNumberOfLevels += numLevelsInGroup;
//				u32 groupFileSize = 0;
//
//				for (flatbuffers::uoffset_t j = 0; j < numLevelsInGroup; j++)
//				{
//					u32 levelFileSize = levelPackData->allLevels()->Get(i)->levels()->Get(j)->levelFileSize();
//					groupFileSize += levelFileSize;
//				}
//
//				if (groupFileSize > maxGroupFileSize)
//				{
//					maxGroupFileSize = groupFileSize;
//				}
//			}
//
//			numLevelsPerGroupLoadedAtOneTime = std::min(numLevelsPerGroupLoadedAtOneTime, maxNumLevelsInGroup);
//
//			levelCacheMemoryNeededInBytes = numLevelGroupsLoadedAtOneTime * maxGroupFileSize + levelPackDataFileSize;	
//		}
//		else
//		{
//			//we need to make some assumptions if we don't have any levels yet
//			
//#if !defined R2_ASSET_PIPELINE
//			R2_CHECK(false, "We should have a level pack file in this case!");
//			return false;
//#else
//			levelPackDataFileSize = Kilobytes(24);
//			numModelSystems = 4;
//			numMaterialSystems = 4;
//			numSoundDefinitionFiles = 4;
//			numLevelsPerGroupLoadedAtOneTime = std::min(numLevelsPerGroupLoadedAtOneTime, maxNumLevelsInGroup);
//			maxGroupFileSize = Megabytes(1);
//			maxLevelSizeInBytes = Kilobytes(64);
//			levelCacheMemoryNeededInBytes = maxGroupFileSize * numLevelGroupsLoadedAtOneTime + levelPackDataFileSize;
//
//			maxNumMaterialsInALevel = 512;
//			maxNumTexturesInAPackForALevel = 1024;
//			maxTextureSizeInAPackForALevel = Megabytes(512);
//			maxNumTexturePacksForAMaterialForALevel = 16;
//			maxTotalNumberOfTextures = 256;
//			maxTexturePackFileSize = Kilobytes(64);
//			maxMaterialPackFileSize = Kilobytes(64);
//
//			maxNumModelsInAPackInALevel = 20;
//			maxModelPackSizeInALevel = Megabytes(64);
//			
//			char levelPackDataName[r2::fs::FILE_PATH_LENGTH];
//			r2::fs::utils::CopyFileName(levelPackPath, levelPackDataName);
//
//			char levelPackRawParentPath[r2::fs::FILE_PATH_LENGTH];
//			r2::util::PathCpy(levelPackRawParentPath, CENG.GetApplication().GetLevelPackDataJSONPath().c_str());
//
//			char levelPackRawPath[r2::fs::FILE_PATH_LENGTH];
//			r2::fs::utils::AppendSubPath(levelPackRawParentPath, levelPackRawPath, levelPackDataName);
//
//			r2::fs::utils::AppendExt(levelPackRawPath, ".json");
//			
//			r2::asset::pln::GenerateEmptyLevelPackFile(levelPackPath, levelPackRawPath);
//#endif
//		}
//
//#ifdef R2_ASSET_PIPELINE
//		//adding more to make level creation more comfortable
//		totalNumberOfLevels = std::max(totalNumberOfLevels, 1000u);
//		levelCacheMemoryNeededInBytes *= 10;
//
//		numModelSystems *= 5;
//		numMaterialSystems *= 5;
//		numSoundDefinitionFiles *= 5;
//#endif
//
//		
//		mNumLevelsPerGroupLoadedAtOneTime = numLevelsPerGroupLoadedAtOneTime;
//		mNumLevelGroupsLoadedAtOneTime = numLevelGroupsLoadedAtOneTime;

		//mMaterialSystems = MAKE_SARRAY(*mArena, r2::draw::MaterialSystem*, numMaterialSystems);
		//mModelSystems = MAKE_SARRAY(*mArena, r2::draw::ModelSystem*, numModelSystems);

		////Might be a problem to have const char* as the type here
		//mSoundDefinitionFilePaths = MAKE_SARRAY(*mArena, const char*, numSoundDefinitionFiles);

		//mLoadedLevels = MAKE_SARRAY(*mArena, LevelGroup, numLevelGroupsLoadedAtOneTime);

		///*for (u32 i = 0; i < numLevelGroupsLoadedAtOneTime; ++i)
		//{
		//	LevelGroup levelGroup = MAKE_SARRAY(*mArena, Level*, mNumLevelsPerGroupLoadedAtOneTime);
		//	r2::sarr::Push(*mLoadedLevels, levelGroup);
		//}*/

		//mGroupMap = MAKE_SARRAY(*mArena, LevelName, numLevelGroupsLoadedAtOneTime);

		
		//if (levelPackDataFile)
		//{
		//	FREE(levelPackDataFile, *MEM_ENG_SCRATCH_PTR);
		//}

		return true;
	}

	void LevelManager::Shutdown()
	{
		//for (s32 i = static_cast<s32>(mNumLevelGroupsLoadedAtOneTime); i >= 0; --i)
		//{
		//	LevelGroup levelGroup = r2::sarr::At(*mLoadedLevels, i);

		//	UnloadLevelGroup(levelGroup);

		//	r2::sarr::Clear(*levelGroup);
		//}

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
	//	for (s32 i = static_cast<s32>(mNumLevelGroupsLoadedAtOneTime); i >= 0; --i)
	//	{
	//		LevelGroup levelGroup = r2::sarr::At(*mLoadedLevels, i);

	//		FREE(levelGroup, *mArena);
	//	}

		//FREE(mLoadedLevels, *mArena);

		//FREE(mSoundDefinitionFilePaths, *mArena);

		//FREE(mModelSystems, *mArena);

		//FREE(mMaterialSystems, *mArena);

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
		//char levelName[r2::fs::FILE_PATH_LENGTH];

	//	r2::fs::utils::CopyFileNameWithParentDirectories(levelURI, levelName, 1); //include the group name
	//	char groupName[r2::fs::FILE_PATH_LENGTH];

	//	r2::fs::utils::GetParentDirectory(levelName, groupName);
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

		Level newLevel;
		newLevel.Init(flatLevelData, levelHandle);

		mSceneGraph.LoadedNewLevel(newLevel);

		r2::shashmap::Set(*mLoadedLevels, levelName, newLevel);

		return &r2::shashmap::Get(*mLoadedLevels, levelName, defaultLevel);
		//if (!mLevelCache || !mLoadedLevels)
		//{
		//	R2_CHECK(false, "We haven't initialized the level manager yet");
		//	return nullptr;
		//}

		//s32 groupIndex = GetGroupIndex(groupName);

		//if (groupIndex == -1 && !r2::sarr::HasRoom(*mLoadedLevels))
		//{
		//	//we don't have any room left in the mLoadedLevels
		//	R2_CHECK(false, "we don't have any room left in the mLoadedLevels");
		//	return nullptr;
		//}
		//else if (groupIndex == -1 && r2::sarr::HasRoom(*mLoadedLevels))
		//{
		//	//make a new entry for the group
		//	groupIndex = AddNewGroupToLoadedLevels(groupName);
		//}

		////now check to see if we have room in the level array of the group
		//if (!(groupName >= 0))
		//{
		//	R2_CHECK(false, "?");
		//	return nullptr;
		//}

		//LevelGroup levelGroupToAddTo = r2::sarr::At(*mLoadedLevels, groupIndex);
		//
		//if (!r2::sarr::HasRoom(*levelGroupToAddTo))
		//{
		//	R2_CHECK(false, "the level group doesn't have any room for the level to be loaded - need to unload first");
		//	return nullptr;
		//}

		//LevelHandle levelHandle = r2::lvlche::LoadLevelData(*mLevelCache, levelName);

		//if (r2::asset::IsInvalidAssetHandle(levelHandle))
		//{
		//	R2_CHECK(false, "Failed to load the level: %llu", levelName);
		//	return nullptr;
		//}

		//const flat::LevelData* levelData = r2::lvlche::GetLevelData(*mLevelCache, levelHandle);

		//if (levelData == nullptr)
		//{
		//	R2_CHECK(false, "Failed to get the level data for: %llu", levelData);
		//	r2::lvlche::UnloadLevelData(*mLevelCache, levelHandle);
		//	return nullptr;
		//}

		//if (levelData->groupHash() != groupName || levelData->hash() != levelName)
		//{
		//	R2_CHECK(false, "These should be equal - probably an issue with content or how we generate the names");
		//	r2::lvlche::UnloadLevelData(*mLevelCache, levelHandle);
		//	return nullptr;
		//}

		////now we need to go through all of the material, texture, sound, model references and see which ones we need/have already
		////Also shaders need to be loaded as well if they are not already...
		//R2_CHECK(false, "@TODO(Serge): implement");




		//Level* newLevel = ALLOC(Level, *mLevelArena);
		//newLevel->Init(levelData, levelHandle);
		//r2::sarr::Push(*levelGroupToAddTo, newLevel);

		//return newLevel;
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

	LevelName LevelManager::MakeLevelNameFromPath(const char* levelPath)
	{
		char sanitizedPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(levelPath, sanitizedPath);

		char levelAssetName[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::CopyFileNameWithParentDirectories(sanitizedPath, levelAssetName, 1);

		std::string fileName = levelAssetName;
		std::transform(std::begin(fileName), std::end(fileName), std::begin(fileName), (int(*)(int))std::tolower);

		return STRING_ID(fileName.c_str());
	}

	u64 LevelManager::MemorySize(
		u32 maxNumLevels,
		const r2::mem::utils::MemoryProperties& memProperties)
	{
		u64 poolHeaderSize = r2::mem::PoolAllocator::HeaderSize();
		u64 memorySize = 0;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), alignof(Level), poolHeaderSize, memProperties.boundsChecking) * maxNumLevels;
		memorySize += lvlche::MemorySize(maxNumLevels, maxNumLevels * AVG_LEVEL_SIZE, memProperties);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Level>::MemorySize(maxNumLevels * r2::SHashMap<LevelName>::LoadFactorMultiplier()), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += SceneGraph::MemorySize(memProperties);
		//max number of materials in a material pack for 1 level
		/*

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Level), alignof(Level), poolHeaderSize, memProperties.boundsChecking) * maxNumLevelsInAGroup * numGroupsLoadedAtOneTime;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::MaterialSystem*>::MemorySize(numMaterialSystems), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(
			r2::draw::mat::MemorySize(
				memProperties.alignment,
				maxNumMaterialsInALevel,
				maxTexturePackSizeForALevel,
				maxTotalNumberOfTextures,
				maxNumTexturePacksForAMaterialForALevel,
				maxNumTexturesInAPackForALevel, maxMaterialPackFileSize, maxTexturePackFileSize),
			memProperties.alignment,
			memProperties.headerSize,
			memProperties.boundsChecking) * numMaterialSystems;
		
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ModelSystem*>::MemorySize(numModelsSystems), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::draw::modlsys::MemorySize(maxNumModelsInAPackInALevel, maxModelPackSizeInALevel), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<const char*>::MemorySize(numSoundDefinitionFiles), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<LevelGroup>::MemorySize(numGroupsLoadedAtOneTime), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Level*>::MemorySize(maxNumLevelsInAGroup), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) * numGroupsLoadedAtOneTime;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<LevelName>::MemorySize(numGroupsLoadedAtOneTime), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += lvlche::MemorySize(maxNumLevelsInAGroup * numGroupsLoadedAtOneTime, levelCacheSize, memProperties);*/

		return memorySize;
	}

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
	void LevelManager::SaveNewLevelFile(u32 version, const char* binLevelPath, const char* rawJSONPath)
	{
		if (!mLevelCache)
		{
			R2_CHECK(false, "We haven't created the level cache yet!");

			return;
		}

		lvlche::SaveNewLevelFile(*mLevelCache, mSceneGraph.GetECSCoordinator(), version, binLevelPath, rawJSONPath);
	}
#endif
}