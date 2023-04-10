#ifndef __LEVEL_MANAGER_H__
#define __LEVEL_MANAGER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"

namespace flat
{
	struct LevelPackData;
}

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2::draw 
{
	struct MaterialSystem;
	struct ModelSystem;
}

namespace r2
{
	struct LevelCache;

	using LevelGroup = r2::SArray<Level*>*;

	class LevelManager
	{
	public:
		LevelManager();
		~LevelManager();

		bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* levelPackPath, const char* areaName, u32 numLevelGroupsLoadedAtOneTime = 1, u32 numLevelsPerGroupLoadedAtOneTime = 1);
		void Shutdown();

		Level* LoadLevel(const char* levelURI);
		Level* LoadLevel(LevelName groupName, LevelName levelName);
		void UnloadLevel(Level* level);

		LevelGroup LoadLevelGroup(const char* levelGroup);
		LevelGroup LoadLevelGroup(LevelName groupName);
		void UnloadLevelGroup(LevelGroup levelGroup);

		bool IsLevelLoaded(LevelName levelName);
		bool IsLevelLoaded(const char* levelName);

		bool IsLevelGroupLoaded(LevelName groupName);
		bool IsLevelGroupLoaded(const char* levelGroup);

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
		void SaveNewLevelFile(LevelName group, const char* levelURI, const void* data, u32 dataSize);
#endif
		static u64 MemorySize(
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

			u32 maxNumModelsInAPackInALevel,
			u32 maxModelPackSizeInALevel,

			const r2::mem::utils::MemoryProperties& memProperties);

	private:

		s32 GetGroupIndex(LevelName groupName);
		s32 AddNewGroupToLoadedLevels(LevelName groupName);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle; 
		u32 mNumLevelsPerGroupLoadedAtOneTime;
		u32 mNumLevelGroupsLoadedAtOneTime;
		
		r2::mem::StackArena* mArena;
		r2::mem::PoolArena* mLevelArena;

		r2::SArray<r2::draw::MaterialSystem*>* mMaterialSystems;
		r2::SArray<r2::draw::ModelSystem*>* mModelSystems;
		r2::SArray<const char*>* mSoundDefinitionFilePaths;
		r2::SArray<LevelGroup>* mLoadedLevels;
		r2::SArray<LevelName>* mGroupMap;

		LevelCache* mLevelCache;
	};
}

#endif // __LEVEL_MANAGER_H__
