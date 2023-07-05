#ifndef __ECS_WORLD_H__
#define __ECS_WORLS_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Game/SceneGraph/SceneGraph.h"

namespace r2
{
	class LevelManager;
	class Level;
}

namespace flat
{
	struct LevelData;
}

namespace r2::ecs
{
	class ECSCoordinator;
	class RenderSystem;
	class SkeletalAnimationSystem;

#ifdef R2_DEBUG
	class DebugBonesRenderSystem;
	class DebugRenderSystem;
#endif

	class ECSWorld
	{
	public:

		ECSWorld();
		~ECSWorld();

		bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems);
		void Shutdown();

		void Update();
		void Render();

		bool LoadLevel(const Level& level, const flat::LevelData* levelData);
		bool UnloadLevel(const Level& level);

		r2::ecs::ECSCoordinator* GetECSCoordinator() const;
		SceneGraph& GetSceneGraph();

		u64 MemorySize(u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems);

	private:

		static const u32 ALIGNMENT = 16;
		
		void RegisterEngineComponents();
		void RegisterEngineSystems();
		void UnRegisterEngineComponents();
		void UnRegisterEngineSystems();

		void* HydrateRenderComponents(void* tempRenderComponents);
		void* HydrateSkeletalAnimationComponents(void* tempSkeletalAnimationComponents);
		void* HydrateInstancedSkeletalAnimationComponents(void* tempInstancedSkeletalAnimationComponents);
		void* HydrateInstancedTransformComponents(void* tempInstancedTransformComponents);


		void PostLoadLevel(const Level& level, const flat::LevelData* levelData);

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle;
		r2::mem::StackArena* mArena;
		r2::ecs::ECSCoordinator* mECSCoordinator;
		r2::ecs::RenderSystem* moptrRenderSystem;
		r2::ecs::SkeletalAnimationSystem* moptrSkeletalAnimationSystem;
#ifdef R2_DEBUG
		r2::ecs::DebugRenderSystem* moptrDebugRenderSystem;
		r2::ecs::DebugBonesRenderSystem* moptrDebugBonesRenderSystem;
#endif

		SceneGraph mSceneGraph;

		//This is temporary since we don't know how much memory will be needed for the component allocations
		//will need to change this later
		r2::mem::MallocArena mMallocArena;
		std::vector<void*> mComponentAllocations;
	};
}

#endif
