
#include "r2pch.h"
#include "r2/Game/Level/LevelManager.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Game/Level/LevelCache.h"
#include "r2/Render/Model/Material.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#endif // R2_ASSET_PIPELINE


namespace r2
{
	LevelManager::LevelManager()
		:mMemoryAreaHandle(r2::mem::MemoryArea::Invalid)
		,mSubAreaHandle(r2::mem::MemoryArea::SubArea::Invalid)
		,mArena(nullptr)
		,mLevelCache(nullptr)
		,mMaterialSystems(nullptr)
		,mModelSystems(nullptr)
		,mSoundDefinitionFilePaths(nullptr)
		,mLoadedLevels(nullptr)
		,mNumLevelsPerGroupLoadedAtOneTime(0)
		,mNumLevelGroupsLoadedAtOneTime(0)
	{
	}

	LevelManager::~LevelManager()
	{
	}

	bool LevelManager::Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* levelPackPath, const char* areaName, u32 numLevelGroupsLoadedAtOneTime, u32 numLevelsPerGroupLoadedAtOneTime)
	{
		R2_CHECK(levelPackPath != nullptr, "We should have a proper path");
		R2_CHECK(strlen(levelPackPath) > 0, "We should have path that's more than 0");
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "We need a valid memory area");

		//We always have to have atleast 1 level group
		if (numLevelGroupsLoadedAtOneTime == 0)
		{
			numLevelGroupsLoadedAtOneTime = 1;
		}

		if (numLevelsPerGroupLoadedAtOneTime == 0)
		{
			numLevelsPerGroupLoadedAtOneTime = 1;
		}

		u32 totalNumberOfLevels = 0;
		u32 maxNumLevelsInGroup = 0;
		u32 maxLevelSizeInBytes = 0;
		u32 maxGroupSizeInBytes = 0;
		u64 levelPackDataFileSize = 0;
		u32 maxGroupFileSize = 0;
		u32 levelCacheMemoryNeededInBytes = 0;
		u32 numModelSystems = 0;
		u32 numMaterialSystems = 0;
		u32 numSoundDefinitionFiles = 0;


		u32 maxNumMaterialsInALevel = 0;
		u32 maxNumTexturesInAPackForALevel = 0;
		u32 maxTextureSizeInAPackForALevel = 0;
		u32 maxNumTexturePacksForAMaterialForALevel = 0;
		u32 maxTotalNumberOfTextures = 0;
		u32 maxTexturePackFileSize = 0;
		u32 maxMaterialPackFileSize = 0;

		void* levelPackDataFile = nullptr;
		const flat::LevelPackData* levelPackData = nullptr;

		if (r2::fs::FileSystem::FileExists(levelPackPath))
		{
			levelPackDataFile = r2::fs::ReadFile(*MEM_ENG_SCRATCH_PTR, levelPackPath, levelPackDataFileSize);

			levelPackData = flat::GetLevelPackData(levelPackDataFile);

			maxNumLevelsInGroup = levelPackData->maxLevelsInAGroup();
			maxLevelSizeInBytes = levelPackData->maxLevelSizeInBytes();
			maxGroupSizeInBytes = levelPackData->maxGroupSizeInBytes();
			numModelSystems = levelPackData->maxNumAnimModelPackReferencesInALevel() + levelPackData->maxNumStaticModelPackReferencesInALevel();
			numMaterialSystems = levelPackData->maxNumMaterialPackReferencesInALevel();
			numSoundDefinitionFiles = levelPackData->maxNumSoundReferencesInALevel() + levelPackData->maxNumMusicReferencesInALevel();

			maxNumMaterialsInALevel = levelPackData->maxNumMaterialsInALevel();
			maxNumTexturesInAPackForALevel = levelPackData->maxNumTexturesInAPackForALevel();
			maxTextureSizeInAPackForALevel = levelPackData->maxTextureSizeInAPackForALevel();
			maxNumTexturePacksForAMaterialForALevel = levelPackData->maxNumTexturePacksForAMaterialForALevel();
			maxTotalNumberOfTextures = levelPackData->maxTotalNumberOfTextures();
			maxTexturePackFileSize = levelPackData->maxTexturePackFileSize();
			maxMaterialPackFileSize = levelPackData->maxMaterialPackFileSize();

			//now figure out how many files to make
			for (flatbuffers::uoffset_t i = 0; i < levelPackData->allLevels()->size(); ++i)
			{
				auto numLevelsInGroup = levelPackData->allLevels()->Get(i)->levels()->size();
				totalNumberOfLevels += numLevelsInGroup;
				u32 groupFileSize = 0;

				for (flatbuffers::uoffset_t j = 0; j < numLevelsInGroup; j++)
				{
					u32 levelFileSize = levelPackData->allLevels()->Get(i)->levels()->Get(j)->levelFileSize();
					groupFileSize += levelFileSize;
				}

				if (groupFileSize > maxGroupFileSize)
				{
					maxGroupFileSize = groupFileSize;
				}
			}

			numLevelsPerGroupLoadedAtOneTime = std::min(numLevelsPerGroupLoadedAtOneTime, maxNumLevelsInGroup);

			levelCacheMemoryNeededInBytes = numLevelGroupsLoadedAtOneTime * maxGroupFileSize + levelPackDataFileSize;	
		}
		else
		{
			//we need to make some assumptions if we don't have any levels yet
			
#if !defined R2_ASSET_PIPELINE
			R2_CHECK(false, "We should have a level pack file in this case!");
			return false;
#else
			levelPackDataFileSize = Kilobytes(24);
			numModelSystems = 4;
			numMaterialSystems = 4;
			numSoundDefinitionFiles = 4;
			numLevelsPerGroupLoadedAtOneTime = std::min(numLevelsPerGroupLoadedAtOneTime, maxNumLevelsInGroup);
			maxGroupFileSize = Megabytes(1);
			maxLevelSizeInBytes = Kilobytes(64);
			levelCacheMemoryNeededInBytes = maxGroupFileSize * numLevelGroupsLoadedAtOneTime + levelPackDataFileSize;

			maxNumMaterialsInALevel = 512;
			maxNumTexturesInAPackForALevel = 1024;
			maxTextureSizeInAPackForALevel = Megabytes(512);
			maxNumTexturePacksForAMaterialForALevel = 16;
			maxTotalNumberOfTextures = 256;
			maxTexturePackFileSize = Kilobytes(64);
			maxMaterialPackFileSize = Kilobytes(64);
			
			char levelPackDataName[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::CopyFileName(levelPackPath, levelPackDataName);

			char levelPackRawParentPath[r2::fs::FILE_PATH_LENGTH];
			r2::util::PathCpy(levelPackRawParentPath, CENG.GetApplication().GetLevelPackDataJSONPath().c_str());

			char levelPackRawPath[r2::fs::FILE_PATH_LENGTH];
			r2::fs::utils::AppendSubPath(levelPackRawParentPath, levelPackRawPath, levelPackDataName);

			r2::fs::utils::AppendExt(levelPackRawPath, ".json");
			
			r2::asset::pln::GenerateEmptyLevelPackFile(levelPackPath, levelPackRawPath);
#endif
			


			
		}

#ifdef R2_ASSET_PIPELINE
		//adding more to make level creation more comfortable
		totalNumberOfLevels = std::max(totalNumberOfLevels, 1000u);
		levelCacheMemoryNeededInBytes *= 10;

		numModelSystems *= 5;
		numMaterialSystems *= 5;
		numSoundDefinitionFiles *= 5;
#endif

		r2::mem::utils::MemoryProperties memProperties;
		memProperties.alignment = 16;

#ifdef R2_DEBUG
		memProperties.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		memProperties.headerSize = r2::mem::StackAllocator::HeaderSize();

		r2::mem::MemoryArea* memoryArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
		R2_CHECK(memoryArea != nullptr, "Memory area is null?");

		u64 subAreaSize = MemorySize(
			numLevelGroupsLoadedAtOneTime,
			numLevelsPerGroupLoadedAtOneTime,
			levelCacheMemoryNeededInBytes,
			numModelSystems,
			numMaterialSystems,
			numSoundDefinitionFiles,
			maxNumMaterialsInALevel,
			maxNumTexturesInAPackForALevel,
			maxTextureSizeInAPackForALevel,
			maxNumTexturePacksForAMaterialForALevel,
			maxTotalNumberOfTextures,
			maxTexturePackFileSize,
			maxMaterialPackFileSize,

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

		mMemoryAreaHandle = memoryAreaHandle;
		mSubAreaHandle = subAreaHandle;
		mNumLevelsPerGroupLoadedAtOneTime = numLevelsPerGroupLoadedAtOneTime;
		mNumLevelGroupsLoadedAtOneTime = numLevelGroupsLoadedAtOneTime;

		mMaterialSystems = MAKE_SARRAY(*mArena, r2::draw::MaterialSystem*, numMaterialSystems);
		mModelSystems = MAKE_SARRAY(*mArena, r2::draw::ModelSystem*, numModelSystems);

		//Might be a problem to have const char* as the type here
		r2::SArray<const char*>* mSoundDefinitionFilePaths = MAKE_SARRAY(*mArena, const char*, numSoundDefinitionFiles);

		r2::SArray<LevelGroup>* mLoadedLevels = MAKE_SARRAY(*mArena, LevelGroup, numLevelGroupsLoadedAtOneTime);

		for (u32 i = 0; i < numLevelGroupsLoadedAtOneTime; ++i)
		{
			LevelGroup levelGroup = MAKE_SARRAY(*mArena, Level*, mNumLevelsPerGroupLoadedAtOneTime);
			r2::sarr::Push(*mLoadedLevels, levelGroup);
		}



		u32 levelCacheMemorySizeNeededInBytes = lvlche::MemorySize(totalNumberOfLevels, levelCacheMemoryNeededInBytes, memProperties);

		if (!(mArena->UnallocatedBytes() > levelCacheMemorySizeNeededInBytes))
		{
			R2_CHECK(false, "We don't have enough memory for the level cache. We have: %llu, but we need: %llu, difference: %llu\n",
				mArena->UnallocatedBytes(), levelCacheMemorySizeNeededInBytes, levelCacheMemorySizeNeededInBytes - mArena->UnallocatedBytes());
			return false;
		}

		r2::mem::utils::MemBoundary levelCacheBoundary = MAKE_BOUNDARY(*mArena, levelCacheMemorySizeNeededInBytes, memProperties.alignment);

		mLevelCache = lvlche::CreateLevelCache(levelCacheBoundary, levelPackPath, levelPackData, totalNumberOfLevels, maxGroupFileSize, maxNumLevelsInGroup, maxLevelSizeInBytes, maxGroupSizeInBytes);

		R2_CHECK(mLevelCache != nullptr, "We couldn't create the level cache correctly");

		if (levelPackDataFile)
		{
			FREE(levelPackDataFile, *MEM_ENG_SCRATCH_PTR);
		}

		return true;
	}

	void LevelManager::Shutdown()
	{
		for (s32 i = static_cast<s32>(mNumLevelGroupsLoadedAtOneTime); i >= 0; --i)
		{
			LevelGroup levelGroup = r2::sarr::At(*mLoadedLevels, i);

			UnloadLevelGroup(levelGroup);

			r2::sarr::Clear(*levelGroup);
		}

		mem::utils::MemBoundary levelCacheBoundary = mLevelCache->mLevelCacheBoundary;

		lvlche::Shutdown(mLevelCache);

		FREE(levelCacheBoundary.location, *mArena);

		for (s32 i = static_cast<s32>(mNumLevelGroupsLoadedAtOneTime); i >= 0; --i)
		{
			LevelGroup levelGroup = r2::sarr::At(*mLoadedLevels, i);

			FREE(levelGroup, *mArena);
		}

		FREE(mLoadedLevels, *mArena);

		FREE(mSoundDefinitionFilePaths, *mArena);

		FREE(mModelSystems, *mArena);

		FREE(mMaterialSystems, *mArena);

		FREE_EMPLACED_ARENA(mArena);

		//@TODO(Serge): should we have to remove the sub areas?
	}

	r2::Level* LevelManager::LoadLevel(const char* level)
	{
		return nullptr;
	}

	r2::Level* LevelManager::LoadLevel(LevelName levelName)
	{
		return nullptr;
	}

	void LevelManager::UnloadLevel(Level* level)
	{

	}

	r2::LevelGroup LevelManager::LoadLevelGroup(const char* levelGroup)
	{
		return nullptr;
	}

	r2::LevelGroup LevelManager::LoadLevelGroup(LevelName groupName)
	{
		return nullptr;
	}

	void LevelManager::UnloadLevelGroup(LevelGroup levelGroup)
	{

	}

	bool LevelManager::IsLevelLoaded(LevelName levelName)
	{
		return false;
	}

	bool LevelManager::IsLevelLoaded(const char* levelName)
	{
		return false;
	}

	bool LevelManager::IsLevelGroupLoaded(LevelName groupName)
	{
		return false;
	}

	bool LevelManager::IsLevelGroupLoaded(const char* levelGroup)
	{
		return false;

	}

	u64 LevelManager::MemorySize(
		u32 numGroupsLoadedAtOneTime,
		u32 maxNumLevelsInAGroup,
		u32 levelCacheSize,
		u32 numModelsSystems,
		u32 numMaterialSystems,
		u32 numSoundDefinitionFiles,

		u32 maxNumMaterialsInALevel,
		u32 maxNumTexturesInAPackForALevel,
		u32 maxTexturePackSizeForALevel,
		u32 maxNumTexturePacksForAMaterialForALevel,
		u32 maxTotalNumberOfTextures,
		u32 maxTexturePackFileSize,
		u32 maxMaterialPackFileSize,

		const r2::mem::utils::MemoryProperties& memProperties)
	{
		/*
		r2::mem::StackArena* mArena;
		r2::SArray<r2::draw::MaterialSystem*>* mMaterialSystems;
		r2::SArray<r2::draw::ModelSystem*>* mModelSystems;
		r2::SArray<const char*>* mSoundDefinitionFilePaths;
		r2::SArray<LevelGroup>* mLoadedLevels;

		LevelCache* mLevelCache;
		*/

		u64 memorySize = 0;

		//max number of materials in a material pack for 1 level

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::MaterialSystem*>::MemorySize(numMaterialSystems), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		//@TODO(Serge): figure out the memory needed for the material systems
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
			memProperties.boundsChecking);
		
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ModelSystem*>::MemorySize(numModelsSystems), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		//@TODO(Serge): figure out the memory needed for the model systems
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<const char*>::MemorySize(numSoundDefinitionFiles), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<LevelGroup>::MemorySize(numGroupsLoadedAtOneTime), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Level*>::MemorySize(maxNumLevelsInAGroup), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking) * numGroupsLoadedAtOneTime;


		memorySize += lvlche::MemorySize(maxNumLevelsInAGroup * numGroupsLoadedAtOneTime, levelCacheSize, memProperties);



		return memorySize;
	}

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
	void LevelManager::SaveNewLevelFile(LevelName group, const char* levelURI, const void* data, u32 dataSize)
	{
		if (!mLevelCache)
		{
			R2_CHECK(false, "We haven't created the level cache yet!");

			return;
		}

		lvlche::SaveNewLevelFile(*mLevelCache, group, levelURI, data, dataSize);
	}
#endif
}