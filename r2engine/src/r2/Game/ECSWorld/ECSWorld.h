#ifndef __ECS_WORLD_H__
#define __ECS_WORLD_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/Allocators/MallocAllocator.h"
#include "r2/Game/SceneGraph/SceneGraph.h"
#include "r2/Game/ECS/Component.h"
#include "r2/Platform/IO.h"
#include "r2/Game/Player/Player.h"
#include "r2/Game/ECS/System.h"

#define ECS_WORLD_ALLOC(ecsWorld, T) r2::ecs::ECSWorldAlloc<T>(ecsWorld, __FILE__, __LINE__, "" )
#define ECS_WORLD_ALLOC_PARAMS(ecsWorld, T, ...) r2::ecs::ECSWorldAllocParams<T>(ecsWorld, __FILE__, __LINE__, "", __VA_ARGS__)
#define ECS_WORLD_ALLOC_BYTES(ecsWorld, numBytes, alignment) r2::ecs::ECSWorldAllocBytes(ecsWorld, numBytes, alignment, __FILE__, __LINE__, "")
#define ECS_WORLD_MAKE_SARRAY(ecsWorld, T, capacity) r2::ecs::ECSWorldMakeSArray<T>(ecsWorld, capacity, __FILE__, __LINE__, "")
#define ECS_WORLD_MAKE_SHASHMAP(ecsWorld, T, capacity) r2::ecs::ECSWorldMakeSHashMap<T>(ecsWorld, capacity, __FILE__, __LINE__, "")
#define ECS_WORLD_FREE(ecsWorld, objPtr) r2::ecs::ECSWorldFree(ecsWorld, objPtr, __FILE__, __LINE__, "")

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
	class AudioListenerSystem;
	class AudioEmitterSystem;
	class SceneGraphSystem;
	class SceneGraphTransformUpdateSystem;
	class LightingUpdateSystem;

#ifdef R2_DEBUG
	class DebugBonesRenderSystem;
	class DebugRenderSystem;
#endif

	class ECSWorld
	{
	public:

		ECSWorld();
		~ECSWorld();

		bool Init(r2::mem::MemoryArea::Handle memoryAreaHandle, u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems, u64 auxMemory);
		void Shutdown();

		void Update();
		void Render();

		bool LoadLevel(const Level& level, const flat::LevelData* levelData);
		bool UnloadLevel(const Level& level);

//		void SetPlayerInputType(PlayerID playerID, const r2::io::InputType& inputType);

		r2::ecs::ECSCoordinator* GetECSCoordinator();
		r2::ecs::SceneGraph& GetSceneGraph();

		u64 MemorySize(u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems, u64 auxMemory);

		template<typename Component>
		void RegisterComponent(const char* componentName, bool shouldSerialize, bool isInstanced, FreeComponentFunc freeComponentFunc)
		{
			mECSCoordinator->RegisterComponent<mem::StackArena, Component>(*mArena, componentName, shouldSerialize, isInstanced, freeComponentFunc);
		}

		template<typename Component>
		void UnRegisterComponent()
		{
			mECSCoordinator->UnRegisterComponent<mem::StackArena, Component>(*mArena);
		}

		//@TODO(Serge): clean up the register appsystem vs register system interface - there should only be one that is public
		//				doing this right now since it makes things simpler 
		void RegisterAppSystem(System* system, s32 sortOrder, bool runInEditor);

		template<typename SystemType>
		SystemType* RegisterSystem(ecs::Signature signature)
		{
			SystemType* newSystem = mECSCoordinator->RegisterSystem<r2::mem::StackArena, SystemType>(*mArena);
			mECSCoordinator->SetSystemSignature<SystemType>(signature);

			return newSystem;
		}

		template<typename SystemType>
		void UnRegisterSystem()
		{
			mECSCoordinator->UnRegisterSystem<mem::StackArena, SystemType>(*mArena);
		}

		//@NOTE(Serge): no one should use this except the below helper methods
		struct ECSWorldAllocation
		{
			void* memPtr;
		};

		inline r2::mem::MallocArena& GetECSComponentArena() { return mMallocArena; }
		inline std::vector<ECSWorldAllocation>& GetComponentAllocations() { return mComponentAllocations; }
	private:

		static const u32 ALIGNMENT = 16;
		
		void RegisterEngineComponents();
		void RegisterEngineSystems();
		void UnRegisterEngineComponents();
		void UnRegisterEngineSystems();

		void FreeRenderComponent(void* renderComponent);
		void FreeSkeletalAnimationComponent(void* skeletalAnimationComponent);
		void FreeInstancedSkeletalAnimationComponent(void* instancedSkeletalAnimationComponent);
		void FreeInstancedTransformComponent(void* instancedTransformComponent);
		void FreePointLightComponent(void* pointLightComponentData);
		void FreeSpotLightComponent(void* spotLightComponentData);

#ifdef R2_DEBUG
		void FreeDebugBoneComponent(void* debugBoneComponent);
		void FreeInstancedDebugRenderComponent(void* instancedDebugRenderComponent);
		void FreeInstancedDebugBoneComponent(void* instancedDebugBoneComponent);

#endif // R2_DEBUG
#ifdef R2_EDITOR
		void FreeSelectionComponent(void* selectionComponent);
#endif

		void PostLoadLevel(const Level& level, const flat::LevelData* levelData);
		r2::io::InputType GetInputTypeForPlayerID(PlayerID playerID);

		struct AppSystem
		{
			ecs::System* system;
			int sortOrder;
			bool runInEditor;
		};

		r2::SArray<AppSystem>* mAppSystems = nullptr;

		r2::mem::MemoryArea::Handle mMemoryAreaHandle;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle;
		r2::mem::StackArena* mArena;
		r2::ecs::ECSCoordinator* mECSCoordinator;
		r2::ecs::RenderSystem* moptrRenderSystem;
		r2::ecs::SkeletalAnimationSystem* moptrSkeletalAnimationSystem;

		r2::ecs::AudioListenerSystem* moptrAudioListenerSystem;
		r2::ecs::AudioEmitterSystem* moptrAudioEmitterSystem;

		r2::ecs::SceneGraphSystem* moptrSceneGraphSystem;
		r2::ecs::SceneGraphTransformUpdateSystem* moptrSceneGraphTransformUpdateSystem;

		r2::ecs::LightingUpdateSystem* moptrLightingUpdateSystem;

#ifdef R2_DEBUG
		r2::ecs::DebugRenderSystem* moptrDebugRenderSystem;
		r2::ecs::DebugBonesRenderSystem* moptrDebugBonesRenderSystem;
#endif

		r2::ecs::SceneGraph mSceneGraph;


		//Player Input Mapping
	//	r2::io::InputType mPlayerInputMappings[Engine::NUM_PLATFORM_CONTROLLERS]; 

		//@TEMPORARY: This is temporary since we don't know how much memory will be needed for the component allocations
		//will need to change this later
		r2::mem::MallocArena mMallocArena;
		std::vector<ECSWorldAllocation> mComponentAllocations;
	};

	template<typename T>
	inline T* ECSWorldAlloc(ECSWorld& ecsWorld, const char* file, u32 line, const char* desc)
	{
		T* newObj = ALLOC_VERBOSE(T, ecsWorld.GetECSComponentArena(), file, line, desc);
		ecsWorld.GetComponentAllocations().push_back({ newObj });

		return newObj;
	}

	template<typename T, class... Args>
	inline T* ECSWorldAllocParams(ECSWorld& ecsWorld, const char* file, u32 line, const char* desc, Args&&... args)
	{
		T* newObj = ALLOC_PARAMS_VERBOSE(T, ecsWorld.GetECSComponentArena(), file, line, desc, std::forward<Args>(args)...);
		ecsWorld.GetComponentAllocations().push_back({ newObj });
		return newObj;
	}

	inline byte* ECSWorldAllocBytes(ECSWorld& ecsWorld, u32 numBytes, u32 alignment, const char* file, u32 line, const char* desc)
	{
		byte* bytes = ALLOC_BYTESN_VERBOSE(ecsWorld.GetECSComponentArena(), numBytes, alignment, file, line, desc);
		ecsWorld.GetComponentAllocations().push_back({static_cast<void*>(bytes)});
		return bytes;
	}

	template<typename T>
	inline SArray<T>* ECSWorldMakeSArray(ECSWorld& ecsWorld, u32 capacity, const char* file, u32 line, const char* desc)
	{
		SArray<T>* sarray = MAKE_SARRAY_VERBOSE(ecsWorld.GetECSComponentArena(), T, capacity, file, line, desc);
		ecsWorld.GetComponentAllocations().push_back({ sarray });
		return sarray;
	}

	template<typename T>
	inline SHashMap<T>* ECSWorldMakeSHashMap(ECSWorld& ecsWorld, u32 capacity, const char* file, u32 line, const char* desc)
	{
		const auto realCapacity = SHashMap<u32>::LoadFactorMultiplier()* capacity;
		SHashMap<T>* map = MAKE_SHASHMAP_VERBOSE(ecsWorld.GetECSComponentArena(), T, realCapacity, file, line, desc);
		ecsWorld.GetComponentAllocations().push_back({ map });
		return map;
	}

	inline void ECSWorldFree(ECSWorld& ecsWorld, void* objPtr, const char* file, u32 line, const char* desc)
	{
		auto& componentAllocations = ecsWorld.GetComponentAllocations();

		auto iter = std::find_if(componentAllocations.begin(), componentAllocations.end(), [objPtr](const ECSWorld::ECSWorldAllocation& allocation) {
			return allocation.memPtr == objPtr;
		});

		if (iter != componentAllocations.end())
		{
			void* memPtr = iter->memPtr;
			
			componentAllocations.erase(iter);
			
			FREE_VERBOSE(memPtr, ecsWorld.GetECSComponentArena(), file, line, desc);
		}
		else
		{
			R2_CHECK(false, "You're trying to deallocate a ptr that isn't in our list of allocations");
		}
	}
}

#endif
