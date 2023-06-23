#ifndef __LEVEL_MANAGER_H__
#define __LEVEL_MANAGER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Game/Level/Level.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

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
	class EditorLevel;

	using LevelGroup = r2::SArray<Level*>*;

	class LevelManager
	{
	public:

		static const u32 MODEL_SYSTEM_SIZE;
		static const u32 ANIMATION_CACHE_SIZE;
		static const u32 MAX_NUM_MODELS;
		static const u32 MAX_NUM_ANIMATIONS;

		LevelManager();
		~LevelManager();

		bool Init(
			r2::mem::MemoryArea::Handle memoryAreaHandle,
			ecs::ECSCoordinator* ecsCoordinator,
			const char* levelPackPath,
			const char* areaName,
			u32 maxNumLevels,
			const char* binLevelOutputPath,
			const char* rawLevelOutputPath);
		void Shutdown();

		void Update();

		const Level* LoadLevel(const char* levelURI);
		const Level* LoadLevel(LevelName levelName);

		const Level* GetLevel(const char* levelURI);
		const Level* GetLevel(LevelName levelName);

		void UnloadLevel(const Level* level);

		bool IsLevelLoaded(LevelName levelName);
		bool IsLevelLoaded(const char* levelURI);

		SceneGraph& GetSceneGraph();
		SceneGraph* GetSceneGraphPtr();

		static LevelName MakeLevelNameFromPath(const char* levelPath);

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
		void SaveNewLevelFile(const EditorLevel& editorLevel);
#endif
		static u64 MemorySize(
			u32 maxNumLevels,
			u32 maxNumModels,
			u32 maxNumAnimations,
			const r2::mem::utils::MemoryProperties& memProperties);

	private:
		u64 GetSubAreaSizeForLevelManager(u32 numLevels, u32 numModels, u32 numAnimations, const r2::mem::utils::MemoryProperties& memProperties) const;

		Level* FindLevel(LevelName levelname, s32& index);

		void LoadLevelData(const flat::LevelData* levelData);
		void UnLoadLevelData(const flat::LevelData* levelData);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle; 
		u32 mMaxNumLevels;
		
		r2::mem::StackArena* mArena;
		r2::mem::PoolArena* mLevelArena;

		r2::SArray<Level>* mLoadedLevels;

		SceneGraph mSceneGraph;	
		char mBinOutputPath[r2::fs::FILE_PATH_LENGTH];
		char mRawOutputPath[r2::fs::FILE_PATH_LENGTH];
	};
}

#endif // __LEVEL_MANAGER_H__
