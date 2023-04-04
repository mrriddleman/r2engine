#ifndef __LEVEL_MANAGER_H__
#define __LEVEL_MANAGER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Game/Level/Level.h"

namespace r2::mem
{
	class StackArena;
}

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

		bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, const char* levelPackPath, u32 numLevelsLoadedAtOneTime = 1, u32 numGroupLevelsLoadedAtOneTime = 1);
		void Shutdown();

		Level* LoadLevel(const char* level);
		Level* LoadLevel(LevelName levelName);
		void UnloadLevel(Level* level);

		LevelGroup LoadLevelGroup(const char* levelGroup);
		LevelGroup LoadLevelGroup(LevelName groupName);
		void UnloadLevelGroup(LevelGroup levelGroup);

		bool IsLevelLoaded(LevelName levelName);
		bool IsLevelLoaded(const char* levelName);

		bool IsLevelGroupLoaded(LevelName groupName);
		bool IsLevelGroupLoaded(const char* levelGroup);

#if defined R2_ASSET_PIPELINE && R2_EDITOR
		void SaveNewLevelFile(LevelName group, const char* levelURI, flat::LevelData* levelData);
#endif

	private:
		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle;
		r2::mem::StackArena* mSubAreaArena;
		LevelCache* mLevelCache;

		r2::SArray<r2::draw::MaterialSystem*>* mMaterialSystems;
		r2::SArray<r2::draw::ModelSystem*>* mModelSystems;
		r2::SArray<const char*>* mSoundDefinitionFilePaths;

		r2::SArray<LevelGroup>* mLoadedLevels;

		u32 mNumLevelsLoadedAtOneTime;
		u32 mNumLevelGroupsLoadedAtOneTime;
	};
}

#endif // __LEVEL_MANAGER_H__
