#ifndef __LEVEL_MANAGER_H__
#define __LEVEL_MANAGER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"

namespace flat
{
	struct LevelPackData;
}

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2
{
	struct LevelCache;
	class GameAssetManager;

	using LevelGroup = r2::SArray<Level*>*;

	class LevelManager
	{
	public:

		static const u32 MODEL_SYSTEM_SIZE;
		static const u32 ANIMATION_CACHE_SIZE;
		static const u32 MAX_NUM_MODELS;
		static const u32 MAX_NUM_ANIMATIONS;
		static const u32 MAX_NUM_TEXTURE_PACKS;

		LevelManager();
		~LevelManager();

		bool Init(
			r2::mem::MemoryArea::Handle memoryAreaHandle,
			const char* areaName,
			u32 maxNumLevels,
			const char* binLevelOutputPath,
			const char* rawLevelOutputPath);
		void Shutdown();

		Level* MakeNewLevel(const char* levelNameStr, const char* groupName, LevelName levelName);

		Level* LoadLevel(const char* levelURI);
		Level* LoadLevel(LevelName levelName);

		Level* GetLevel(const char* levelURI);
		Level* GetLevel(LevelName levelName);

		void UnloadLevel(const Level* level);

		bool IsLevelLoaded(LevelName levelName);
		bool IsLevelLoaded(const char* levelURI);

		Level* ReloadLevel(const char* levelURI);
		Level* ReloadLevel(LevelName levelName);

		static LevelName MakeLevelNameFromPath(const char* levelPath);

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
		void SaveNewLevelFile(const Level& editorLevel);
#endif
		static u64 MemorySize(
			u32 maxNumLevels,
			u32 maxNumModels,
			u32 maxNumAnimations,
			u32 maxNumTexturePacks,
			u32 maxNumEntities,
			const r2::mem::utils::MemoryProperties& memProperties);

	private:
		
		Level* FindLoadedLevel(LevelName levelname, s32& index);
		void LoadLevelData(Level& level, const flat::LevelData* levelData);
		void UnLoadLevelData(const Level& level);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle; 
		u32 mMaxNumLevels;
		
		r2::mem::StackArena* mArena;
		r2::mem::FreeListArena* mLevelArena;

		r2::SArray<Level>* mLoadedLevels;

		char mBinOutputPath[r2::fs::FILE_PATH_LENGTH];
		char mRawOutputPath[r2::fs::FILE_PATH_LENGTH];
	};
}

#endif // __LEVEL_MANAGER_H__
