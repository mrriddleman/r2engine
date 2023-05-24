
#include "r2pch.h"
#include "r2/Game/Level/LevelCache.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Game/Level/LevelPack_generated.h"
#include "r2/Game/Level/LevelData_generated.h"
#include "r2/Core/Assets/AssetLib.h"
#include "r2/Core/Assets/AssetCache.h"
#include "r2/Core/Assets/AssetBuffer.h"
#include "r2/Utils/Hash.h"

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/LevelPackDataUtils.h"
#endif

#include <filesystem>

namespace r2::lvlche
{
	constexpr u32 NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_LEVEL_NAME = 1; //we want the group name as well
	constexpr u32 ALIGNMENT = 16;

	LevelCache* CreateLevelCache(const r2::mem::utils::MemBoundary& levelCacheBoundary, const char* levelDirectory, u32 levelCapacity, u32 cacheSize)
	{
		r2::asset::FileList fileList = r2::asset::lib::MakeFileList(levelCapacity);

		R2_CHECK(levelDirectory != nullptr, "Level directory is nullptr");

		//@Temporary
		for (auto& file : std::filesystem::recursive_directory_iterator(levelDirectory))
		{
			if (!(file.is_regular_file() && file.file_size() > 0))
			{
				continue;
			}

			r2::asset::RawAssetFile* levelFile = r2::asset::lib::MakeRawAssetFile(file.path().string().c_str(), NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_LEVEL_NAME);
			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)levelFile);
		}

		/*if (initialLevelPackData)
		{
			r2::asset::RawAssetFile* levelPackRawFile = r2::asset::lib::MakeRawAssetFile(levelPackDataFilePath);
			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)levelPackRawFile);

			for (flatbuffers::uoffset_t i = 0; i < initialLevelPackData->allLevels()->size(); ++i)
			{
				auto numLevelsInGroup = initialLevelPackData->allLevels()->Get(i)->levels()->size();
				for (flatbuffers::uoffset_t j = 0; j < numLevelsInGroup; j++)
				{
					const char* levelRelPath = initialLevelPackData->allLevels()->Get(i)->levels()->Get(j)->levelPath()->c_str();

					char levelPath[r2::fs::FILE_PATH_LENGTH];
					r2::fs::utils::BuildPathFromCategory(fs::utils::LEVELS, levelRelPath, levelPath);

					r2::asset::RawAssetFile* levelFile = r2::asset::lib::MakeRawAssetFile(levelPath, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_LEVEL_NAME);

					r2::sarr::Push(*fileList, (r2::asset::AssetFile*)levelFile);
				}
			}
		}*/

		R2_CHECK(levelCacheBoundary.location != nullptr, "Passed in an invalid level cache boundary");

		LevelCache* newLevelCache = new (levelCacheBoundary.location) LevelCache();

		R2_CHECK(newLevelCache != nullptr, "Couldn't create the new level cache?");

		newLevelCache->mLevelCacheBoundary = levelCacheBoundary;

		r2::mem::utils::MemBoundary stackArenaBoundary;
		stackArenaBoundary.location = r2::mem::utils::PointerAdd(levelCacheBoundary.location, sizeof(LevelCache));
		stackArenaBoundary.size = levelCacheBoundary.size - sizeof(LevelCache);

		newLevelCache->mArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(stackArenaBoundary);

		newLevelCache->mLevels = MAKE_SARRAY(*newLevelCache->mArena, r2::asset::AssetCacheRecord, r2::sarr::Capacity(*fileList));

		u64 levelCacheSize = r2::asset::AssetCache::TotalMemoryNeeded(newLevelCache->mArena->HeaderSize(), newLevelCache->mArena->FooterSize(), r2::sarr::Capacity(*fileList), cacheSize, ALIGNMENT);
		
		if (!(levelCacheSize < newLevelCache->mArena->UnallocatedBytes()))
		{
			R2_CHECK(false, "We don't have enough space for the level cache - we need at least 1 group level size");
			RESET_ARENA(*newLevelCache->mArena);
			FREE_EMPLACED_ARENA(newLevelCache->mArena);
			return nullptr;
		}

		newLevelCache->mAssetBoundary = MAKE_MEMORY_BOUNDARY_VERBOSE(*newLevelCache->mArena, levelCacheSize, ALIGNMENT, "Level Cache Asset Boundary");

		newLevelCache->mLevelCache = r2::asset::lib::CreateAssetCache(newLevelCache->mAssetBoundary, fileList);

		//char levelPackNameWithExtension[r2::fs::FILE_PATH_LENGTH];

		//r2::fs::utils::CopyFileNameWithExtension(levelPackDataFilePath, levelPackNameWithExtension);

		//newLevelCache->mLevelPackDataAssetHandle = newLevelCache->mLevelCache->LoadAsset(r2::asset::Asset(levelPackNameWithExtension, r2::asset::LEVEL_PACK));

		//R2_CHECK(!r2::asset::IsInvalidAssetHandle(newLevelCache->mLevelPackDataAssetHandle), "We couldn't load the level pack data");
		//
		//newLevelCache->mLevelPackCacheRecord = newLevelCache->mLevelCache->GetAssetBuffer(newLevelCache->mLevelPackDataAssetHandle);

		//R2_CHECK(newLevelCache->mLevelPackCacheRecord.buffer != nullptr, "We couldn't get the asset buffer");

		//newLevelCache->mLevelPackData = flat::GetLevelPackData(newLevelCache->mLevelPackCacheRecord.buffer->Data());

		newLevelCache->mLevelCache->RegisterAssetFreedCallback([newLevelCache](const r2::asset::AssetHandle& handle)
			{
				const auto numLevels = r2::sarr::Size(*newLevelCache->mLevels);

				for (u32 i = 0; i < numLevels; ++i)
				{
					const auto& assetCacheRecord = r2::sarr::At(*newLevelCache->mLevels, i);
					if (r2::asset::AreAssetHandlesEqual({ assetCacheRecord.GetAsset().HashID(), assetCacheRecord.GetAssetCache()->GetSlot()
				}, handle))
					{
						r2::sarr::RemoveAndSwapWithLastElement(*newLevelCache->mLevels, i);
						break;
					}
				}
			});

		return newLevelCache;
	}

	void Shutdown(LevelCache* levelCache)
	{
		if (!levelCache)
		{
			R2_CHECK(false, "Trying to Shutdown a null level cache");
			return;
		}

		const auto numLevelsToReturn = r2::sarr::Size(*levelCache->mLevels);

		for (u32 i = 0; i < numLevelsToReturn; ++i)
		{
			levelCache->mLevelCache->ReturnAssetBuffer(r2::sarr::At(*levelCache->mLevels, i));
		}

		//levelCache->mLevelCache->ReturnAssetBuffer(levelCache->mLevelPackCacheRecord);
		//levelCache->mLevelPackDataAssetHandle = {};
		//levelCache->mLevelPackCacheRecord = {};

		r2::asset::lib::DestroyCache(levelCache->mLevelCache);

		FREE(levelCache->mAssetBoundary.location, *levelCache->mArena);
		FREE(levelCache->mLevels, *levelCache->mArena);
		FREE_EMPLACED_ARENA(levelCache->mArena);
		
		levelCache->~LevelCache();
	}

	u64 MemorySize(u32 maxNumLevels, u32 cacheSizeInBytes, const r2::mem::utils::MemoryProperties& memProperties)
	{
		u64 memorySize = 0;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(LevelCache), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::AssetCacheRecord>::MemorySize(maxNumLevels), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::asset::AssetCache), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::asset::AssetCache::TotalMemoryNeeded(memProperties.headerSize, memProperties.boundsChecking, maxNumLevels, cacheSizeInBytes, memProperties.alignment), memProperties.alignment, memProperties.headerSize, memProperties.boundsChecking);

		return memorySize;
	}

	LevelHandle LoadLevelData(LevelCache& levelCache, const char* levelname)
	{
		if (levelCache.mLevelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the levelCache yet!");
			return {};
		}

		LevelHandle levelHandle = levelCache.mLevelCache->LoadAsset(r2::asset::Asset(levelname, r2::asset::LEVEL));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(levelHandle), "We have an invalid handle for: %s\n", levelname);

		return levelHandle;
	}

	LevelHandle LoadLevelData(LevelCache& levelCache, LevelName levelname)
	{
		if (levelCache.mLevelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the levelCache yet!");
			return {};
		}

		LevelHandle levelHandle = levelCache.mLevelCache->LoadAsset(r2::asset::Asset(levelname, r2::asset::LEVEL));

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(levelHandle), "We have an invalid handle for: %llu\n", levelname);

		return levelHandle;
	}

	LevelHandle LoadLevelData(LevelCache& levelCache, const r2::asset::Asset& levelAsset)
	{
		if (levelCache.mLevelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the levelCache yet!");
			return {};
		}

		LevelHandle levelHandle = levelCache.mLevelCache->LoadAsset(levelAsset);

		R2_CHECK(!r2::asset::IsInvalidAssetHandle(levelHandle), "We have an invalid handle for: %llu\n", levelAsset.HashID());

		return levelHandle;
	}

	const flat::LevelData* GetLevelData(LevelCache& levelCache, const LevelHandle& handle)
	{
		if (levelCache.mLevelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the levelCache yet!");
			return nullptr;
		}

		r2::asset::AssetCacheRecord newRecord = levelCache.mLevelCache->GetAssetBuffer(handle);

		r2::sarr::Push(*levelCache.mLevels, newRecord);

		return flat::GetLevelData(newRecord.GetAssetBuffer()->Data());
	}

	void UnloadLevelData(LevelCache& levelCache, const LevelHandle& handle)
	{
		if (levelCache.mLevelCache == nullptr)
		{
			R2_CHECK(false, "We haven't initialized the levelCache yet!");
			return;
		}

		const auto numLevels = r2::sarr::Size(*levelCache.mLevels);

		for (u32 i = 0; i < numLevels; i++)
		{
			const auto& assetCacheRecord = r2::sarr::At(*levelCache.mLevels, i);
			if (r2::asset::AreAssetHandlesEqual({ assetCacheRecord.GetAsset().HashID(), assetCacheRecord.GetAssetCache()->GetSlot() }, handle))
			{
				levelCache.mLevelCache->ReturnAssetBuffer(assetCacheRecord);
				return;
			}
		}

		R2_CHECK(false, "We couldn't find the asset handle: %llu", handle.handle);
	}

	/*u32 GetNumLevelsInLevelGroup(LevelCache& levelCache, LevelName groupName)
	{
		const auto levelsToSearch = levelCache.mLevelPackData->allLevels();

		for (flatbuffers::uoffset_t i = 0; i < levelsToSearch->size(); ++i)
		{
			if (levelsToSearch->Get(i)->groupHash() == groupName)
			{
				return levelsToSearch->Get(i)->levels()->size();
			}
		}

		return 0;
	}*/

	//u32 GetMaxLevelSizeInBytes(LevelCache& levelCache)
	//{
	//	return levelCache.mMaxLevelSizeInBytes;
	//}

	//u32 GetMaxNumLevelsInAGroup(LevelCache& levelCache)
	//{
	//	return levelCache.mMaxNumLevelsInGroup;
	//}

	//u32 GetMaxGroupSizeInBytes(LevelCache& levelCache)
	//{
	//	return levelCache.mMaxGroupSizeInBytes;
	//}

	//u32 GetNumberOfLevelFiles(LevelCache& levelCache)
	//{
	//	return r2::sarr::Size(*levelCache.mLevels);
	//}

	u32 GetLevelCapacity(LevelCache& levelCache)
	{
		return r2::sarr::Capacity(*levelCache.mLevels);
	}

	bool HasLevel(LevelCache& levelCache, const char* levelURI)
	{
		return HasLevel(levelCache, STRING_ID(levelURI));
	}

	bool HasLevel(LevelCache& levelCache, const LevelName name)
	{
		return HasLevel(levelCache, r2::asset::Asset(name, r2::asset::LEVEL));
	}

	bool HasLevel(LevelCache& levelCache, const r2::asset::Asset& levelAsset)
	{
		return levelCache.mLevelCache->HasAsset(levelAsset);
	}

	//void LoadLevelGroup(LevelCache& levelCache, LevelName groupName, r2::SArray<LevelHandle>& levelHandles)
	//{
	//	if (levelCache.mLevelCache == nullptr)
	//	{
	//		R2_CHECK(false, "We haven't initialized the levelCache yet!");
	//		return;
	//	}

	//	const auto levelsToSearch = levelCache.mLevelPackData->allLevels();

	//	for (flatbuffers::uoffset_t i = 0; i < levelsToSearch->size(); ++i)
	//	{
	//		if (levelsToSearch->Get(i)->groupHash() == groupName)
	//		{
	//			R2_CHECK(r2::sarr::Capacity(levelHandles) - r2::sarr::Size(levelHandles) >= levelsToSearch->Get(i)->levels()->size(), "We don't have enough space to put the handles!");

	//			const auto levelGroup = levelsToSearch->Get(i);

	//			for (flatbuffers::uoffset_t j = 0; j < levelGroup->levels()->size(); ++j)
	//			{
	//				r2::sarr::Push(levelHandles, LoadLevelData(levelCache, levelGroup->levels()->Get(j)->levelName()->c_str()));
	//			}

	//			break;
	//		}
	//	}
	//}

	//void GetLevelDataForGroup(LevelCache& levelCache, const r2::SArray<LevelHandle>& levelHandles, r2::SArray<const flat::LevelData*>& levelGroupData)
	//{
	//	if (levelCache.mLevelCache == nullptr)
	//	{
	//		R2_CHECK(false, "We haven't initialized the levelCache yet!");
	//		return;
	//	}

	//	const auto numHandles = r2::sarr::Size(levelHandles);
	//	R2_CHECK(r2::sarr::Size(levelHandles) <= (r2::sarr::Capacity(levelGroupData) - r2::sarr::Size(levelGroupData)), "We don't have enough space for the levelGroupData");
	//	for (u32 i = 0; i < numHandles; ++i)
	//	{
	//		r2::sarr::Push(levelGroupData, GetLevelData(levelCache, r2::sarr::At(levelHandles, i)));
	//	}
	//}

	//void UnloadLevelGroupData(LevelCache& levelCache, const r2::SArray<LevelHandle>& levelHandles)
	//{
	//	if (levelCache.mLevelCache == nullptr)
	//	{
	//		R2_CHECK(false, "We haven't initialized the levelCache yet!");
	//		return;
	//	}

	//	const auto numHandles = r2::sarr::Size(levelHandles);

	//	for (u32 i = 0; i < numHandles; ++i)
	//	{
	//		UnloadLevelData(levelCache, r2::sarr::At(levelHandles, i));
	//	}
	//}

	void FlushAll(LevelCache& levelCache)
	{
		const auto numLevels = r2::sarr::Size(*levelCache.mLevels);

		for (u32 i = 0; i < numLevels; ++i)
		{
			levelCache.mLevelCache->ReturnAssetBuffer(r2::sarr::At(*levelCache.mLevels, i));
		}

		r2::sarr::Clear(*levelCache.mLevels);

		levelCache.mLevelCache->FlushAll();
	}

#if defined(R2_ASSET_PIPELINE) && defined(R2_EDITOR)
	bool SaveNewLevelFile(
		LevelCache& levelCache,
		const r2::ecs::ECSCoordinator* coordinator,
		u32 version,
		const char* binLevelPath,
		const char* rawJSONPath,
		const r2::asset::FileList modelFiles,
		const r2::asset::FileList animationFiles)
	{

		char sanitizedBinLevelPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(binLevelPath, sanitizedBinLevelPath);

		char sanitizedRawLevelPath[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::SanitizeSubPath(rawJSONPath, sanitizedRawLevelPath);

		char levelURI[r2::fs::FILE_PATH_LENGTH];
		r2::fs::utils::CopyFileNameWithParentDirectories(sanitizedBinLevelPath, levelURI, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_LEVEL_NAME);

		r2::asset::Asset newLevelAsset(levelURI, r2::asset::LEVEL);
		
		const r2::asset::FileList fileList = levelCache.mLevelCache->GetFileList();

		//first check to see if we have asset for this
		if (!levelCache.mLevelCache->HasAsset(newLevelAsset))
		{
			//make a new asset file for the asset cache
			r2::asset::RawAssetFile* newFile = r2::asset::lib::MakeRawAssetFile(sanitizedBinLevelPath, NUM_PARENT_DIRECTORIES_TO_INCLUDE_IN_LEVEL_NAME);

			r2::sarr::Push(*fileList, (r2::asset::AssetFile*)newFile);
		}

		//Write out the new level file
		bool saved = r2::asset::pln::SaveLevelData(coordinator, version, sanitizedBinLevelPath, sanitizedRawLevelPath, modelFiles, animationFiles);
		
		R2_CHECK(saved, "We couldn't save the file: %s\n", sanitizedBinLevelPath);
		//now we have to update the LevelGroupData and LevelPackData


		return saved;
		////first return the cache record
		//levelCache.mLevelCache->ReturnAssetBuffer(levelCache.mLevelPackCacheRecord);
		////force free the asset since we'll still hold onto it even though there are no references
		//levelCache.mLevelCache->FreeAsset(levelCache.mLevelPackDataAssetHandle);

		////don't hold any reference to the record
		//levelCache.mLevelPackDataAssetHandle = {};
		//levelCache.mLevelPackCacheRecord = {};
		//levelCache.mLevelPackData = nullptr;

		////now regenerate the levelPackData
		//{
		//	char binLevelPackDataPath[r2::fs::FILE_PATH_LENGTH];
		//	r2::util::PathCpy(binLevelPackDataPath, r2::sarr::At(*fileList, 0)->FilePath());

		//	char levelPackDataName[r2::fs::FILE_PATH_LENGTH];
		//	r2::fs::utils::CopyFileName(binLevelPackDataPath, levelPackDataName);

		//	char levelPackRawParentPath[r2::fs::FILE_PATH_LENGTH];
		//	r2::util::PathCpy(levelPackRawParentPath, CENG.GetApplication().GetLevelPackDataJSONPath().c_str());

		//	char levelPackRawPath[r2::fs::FILE_PATH_LENGTH];
		//	r2::fs::utils::AppendSubPath(levelPackRawParentPath, levelPackRawPath, levelPackDataName);

		//	r2::fs::utils::AppendExt(levelPackRawPath, ".json");

		//	r2::asset::pln::RegenerateLevelDataFromDirectories(binLevelPackDataPath, levelPackRawPath, CENG.GetApplication().GetLevelPackDataBinPath().c_str(), CENG.GetApplication().GetLevelPackDataJSONPath().c_str());
		//}
		//
		//char levelPackNameWithExtension[r2::fs::FILE_PATH_LENGTH];

		//r2::fs::utils::CopyFileNameWithExtension(r2::sarr::At(*fileList, 0)->FilePath(), levelPackNameWithExtension);

		////now we can reload the level pack
		//levelCache.mLevelPackDataAssetHandle = levelCache.mLevelCache->LoadAsset(r2::asset::Asset(levelPackNameWithExtension, r2::asset::LEVEL_PACK));

		//R2_CHECK(!r2::asset::IsInvalidAssetHandle(levelCache.mLevelPackDataAssetHandle), "We couldn't load the level pack data");

		//levelCache.mLevelPackCacheRecord = levelCache.mLevelCache->GetAssetBuffer(levelCache.mLevelPackDataAssetHandle);

		//R2_CHECK(levelCache.mLevelPackCacheRecord.buffer != nullptr, "We couldn't get the asset buffer");

		//levelCache.mLevelPackData = flat::GetLevelPackData(levelCache.mLevelPackCacheRecord.buffer->Data());

		//R2_CHECK(levelCache.mLevelPackData != nullptr, "We don't have the level pack!");

		//levelCache.mMaxGroupSizeInBytes = levelCache.mLevelPackData->maxGroupSizeInBytes();
		//levelCache.mMaxLevelSizeInBytes = levelCache.mLevelPackData->maxLevelSizeInBytes();
		//levelCache.mMaxNumLevelsInGroup = levelCache.mLevelPackData->maxLevelsInAGroup();
	}
#endif
}