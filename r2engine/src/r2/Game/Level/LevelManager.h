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
	struct ModelCache;
	struct AnimationCache;
}

namespace r2
{
	struct LevelCache;

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

		r2::draw::ModelCache* GetModelSystem();
		r2::draw::AnimationCache* GetAnimationCache();

		static LevelName MakeLevelNameFromPath(const char* levelPath);

#if defined (R2_ASSET_PIPELINE) && defined (R2_EDITOR)
		void SaveNewLevelFile(
			u32 version,
			const char* binLevelPath,
			const char* rawJSONPath,
			const r2::draw::ModelCache& modelSystem,
			const r2::draw::AnimationCache& animationCache);
#endif
		static u64 MemorySize(
			u32 maxNumLevels,
			u32 maxNumModels,
			u32 maxNumAnimations,
			const r2::mem::utils::MemoryProperties& memProperties);

	private:
		u64 GetSubAreaSizeForLevelManager(u32 numLevels, u32 numModels, u32 numAnimations, const r2::mem::utils::MemoryProperties& memProperties) const;

		void AddModelFilesToModelSystem(const flat::LevelData* levelData);
		void AddAnimationFilesToAnimationCache(const flat::LevelData* levelData);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle; 
		u32 mMaxNumLevels;
		
		r2::mem::StackArena* mArena;
		r2::mem::PoolArena* mLevelArena;

		r2::SHashMap<Level>* mLoadedLevels; //LevelName -> LevelHandle

		LevelCache* mLevelCache;

		SceneGraph mSceneGraph;
		
		r2::draw::ModelCache* mModelCache;
		r2::draw::AnimationCache* mAnimationCache;

		//The issue is that material systems as they are now are very rigid and not well thought out
		//Firstly, they include the textures directly which is limiting and makes things harder
		//Second, MaterialSystem is a bad name, really it's 1 MaterialParamPack that doesn't really change
		//What the LevelManager would require is an actual MaterialCache that has many of these packs stored so that it can load/unload them when needed
		//If we break up the materials so that they don't include the textures, we'd need a texture cache as well
		r2::draw::MaterialSystem* mMaterialSystem;

		
	
	};
}

#endif // __LEVEL_MANAGER_H__