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

		bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, ecs::ECSCoordinator* ecsCoordinator, const char* levelPackPath, const char* areaName, u32 maxNumLevels);
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
		void SaveNewLevelFile(u32 version, const char* binLevelPath, const char* rawJSONPath);
#endif
		static u64 MemorySize(
			u32 maxNumLevels,
			const r2::mem::utils::MemoryProperties& memProperties);

	private:

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle; 
		u32 mMaxNumLevels;
		//u32 mMaxNumLevelsLoadedAtOneTime;
		
		r2::mem::StackArena* mArena;
		r2::mem::PoolArena* mLevelArena;

		r2::SHashMap<Level>* mLoadedLevels; //LevelName -> LevelHandle

		LevelCache* mLevelCache;

		SceneGraph mSceneGraph;
		
	};
}

#endif // __LEVEL_MANAGER_H__
