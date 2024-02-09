#ifndef __ECS_COORDINATOR_H__
#define __ECS_COORDINATOR_H__

//https://austinmorlan.com/posts/entity_component_system/#further-reading

#include "r2/Game/ECS/Entity.h"
#include "r2/Game/ECS/ComponentManager.h"
#include "r2/Game/ECS/SystemManager.h"

namespace r2
{
	class Level;
}

namespace flat
{
	struct LevelData;
}

namespace r2::ecs
{
	class ECSWorld;

	class ECSCoordinator
	{
	public:
		ECSCoordinator();
		~ECSCoordinator();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumComponents, u32 maxNumEntities, u32 startingEntity, u32 maxNumSystems)
		{
			mEntityManager = ALLOC(EntityManager, arena);

			R2_CHECK(mEntityManager != nullptr, "Failed to create the EntityManager");

			bool entityManagerInitialized = mEntityManager->Init<ARENA>(arena, maxNumEntities, startingEntity);
			
			R2_CHECK(entityManagerInitialized, "Failed to initialize the EntityManager");

			mComponentManager = ALLOC(ComponentManager, arena);

			R2_CHECK(mComponentManager != nullptr, "Failed to create the ComponentManager");

			bool componentManagerInitialized = mComponentManager->Init<ARENA>(arena, maxNumComponents);

			R2_CHECK(componentManagerInitialized, "Failed to initialize the ComponentManager");

			mSystemManager = ALLOC(SystemManager, arena);

			R2_CHECK(mSystemManager != nullptr, "Failed to create the SystemManager");

			bool systemManagerInitialized = mSystemManager->Init<ARENA>(arena, maxNumSystems);
			
			R2_CHECK(systemManagerInitialized, "Failed to initialize the SystemManager");

			return entityManagerInitialized && componentManagerInitialized && systemManagerInitialized;
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			mSystemManager->Shutdown(arena);
			FREE(mSystemManager, arena);
			mComponentManager->Shutdown(arena);
			FREE(mComponentManager, arena);
			mEntityManager->Shutdown(arena);
			FREE(mEntityManager, arena);

			mEntityManager = nullptr;
			mComponentManager = nullptr;
			mSystemManager = nullptr;
		}

		Entity CreateEntity();
		void DestroyEntity(Entity entity);
		void DestoryAllEntities();
		const r2::SArray<Entity>& GetAllLivingEntities();

		u32 NumLivingEntities() const;

		void LoadAllECSDataFromLevel(ECSWorld& ecsWorld, const Level& level, const flat::LevelData* levelData);
		void UnloadAllECSDataFromLevel(const Level& level);

		template<typename Component>
		bool HasComponent(Entity entity)
		{
			const auto signature = mEntityManager->GetSignature(entity);

			ComponentType componentType = GetComponentType<Component>();

			return signature.test(componentType);
		}

		bool HasComponent(Entity entity, u64 componentTypeHash) const
		{
			const auto signature = mEntityManager->GetSignature(entity);

			ComponentType componentType = mComponentManager->GetComponentType(componentTypeHash);

			return signature.test(componentType);
		}

		template<class ARENA, typename Component>
		void RegisterComponent(ARENA& arena, const char* componentName, bool shouldSerialize, bool isInstanced, FreeComponentFunc freeComponentFunc)
		{
			mComponentManager->RegisterComponentType<ARENA, Component>(arena, componentName, shouldSerialize, isInstanced, freeComponentFunc);
		}

		template<class ARENA, typename Component>
		void UnRegisterComponent(ARENA& arena)
		{
			mComponentManager->UnRegisterComponentType<ARENA, Component>(arena);
		}

		template<typename Component>
		void AddComponent(Entity entity, Component component)
		{
			mComponentManager->AddComponent<Component>(entity, component);

			auto signature = mEntityManager->GetSignature(entity);
			signature.set(mComponentManager->GetComponentType<Component>(), true);
			mEntityManager->SetSignature(entity, signature);
			
			mSystemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename Component>
		void RemoveComponent(Entity entity)
		{
			mComponentManager->RemoveComponent<Component>(entity);

			auto signature = mEntityManager->GetSignature(entity);
			signature.set(mComponentManager->GetComponentType<Component>(), false);
			mEntityManager->SetSignature(entity, signature);

			mSystemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename Component>
		void AddComponentToAllEntities(Component component)
		{
			const r2::SArray<Entity>& entities = mEntityManager->GetCreatedEntities();

			const auto numLivingEntities = r2::sarr::Size(entities);

			for (u32 i = 0; i < numLivingEntities; ++i)
			{
				AddComponent(r2::sarr::At(entities, i), component);
			}
		}

		
		void GetAllEntitiesWithComponent(ComponentType componentType, r2::SArray<ecs::Entity>& entities);

		template<typename Component>
		void AddComponentToAllEntities(Component component, const r2::SArray<ecs::Entity>& entities)
		{
			const auto numLivingEntities = r2::sarr::Size(entities);

			for (u32 i = 0; i < numLivingEntities; ++i)
			{
				AddComponent(r2::sarr::At(entities, i), component);
			}
		}


		template<typename Component>
		Component& GetComponent(Entity entity)
		{
			return mComponentManager->GetComponent<Component>(entity);
		}

		template<typename Component>
		Component* GetComponentPtr(Entity entity)
		{
			return mComponentManager->GetComponentPtr<Component>(entity);
		}

		template<typename Component>
		void SetComponent(Entity entity, const Component& component)
		{
			mComponentManager->SetComponent(entity, component);
		}

		template<typename Component>
		ComponentType GetComponentType()
		{
			return mComponentManager->GetComponentType<Component>();
		}

		template<typename Component>
		u64 GetComponentTypeHash() const
		{
			return mComponentManager->GetComponentTypeHash<Component>();
		}

#ifdef R2_EDITOR
		std::vector<std::string> GetAllRegisteredComponentNames() const
		{
			return mComponentManager->GetAllRegisteredComponentNames();
		}

		std::vector<u64> GetAllRegisteredComponentTypeHashes() const
		{
			return mComponentManager->GetAllRegisteredComponentTypeHashes();
		}

		std::vector<std::string> GetAllRegisteredNonInstancedComponentNames() const
		{
			return mComponentManager->GetAllRegisteredNonInstancedComponentNames();
		}

		std::vector<u64> GetAllRegisteredNonInstancedComponentTypeHashes() const
		{
			return mComponentManager->GetAllRegisteredNonInstancedComponentTypeHashes();
		}
#endif

		template<class ARENA, typename SystemType>
		SystemType* RegisterSystem(ARENA& arena)
		{
			SystemType* newSystem = mSystemManager->RegisterSystem<ARENA, SystemType>(arena);
			newSystem->mnoptrCoordinator = this;
			return newSystem;
		}

		template<class ARENA, typename SystemType>
		void UnRegisterSystem(ARENA& arena)
		{
			mSystemManager->UnRegisterSystem<ARENA, SystemType>(arena);
		}

		template<typename SystemType>
		void SetSystemSignature(Signature signature)
		{
			mSystemManager->SetSignature<SystemType>(signature);
		}

		template<typename SystemType>
		void MoveEntity(u64 fromIndex, u64 toIndex)
		{
			mSystemManager->MoveEntity<SystemType>(fromIndex, toIndex);
		}

		Signature& GetSignature(Entity e);

		void SerializeECS (
			flatbuffers::FlatBufferBuilder& builder,
			std::vector<flatbuffers::Offset<flat::EntityData>>& entityVec,
			std::vector<flatbuffers::Offset<flat::ComponentArrayData>>& componentData) const;

		static u64 MemorySize(u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems, u64 alignment, u32 headerSize, u32 boundsChecking);

		template <typename SystemType>
		static u64 MemorySizeOfSystemType(const r2::mem::utils::MemoryProperties& memProperties)
		{
			return SystemManager::MemorySizeOfSystemType<SystemType>(memProperties);
		}

	private:

		EntityManager* mEntityManager;
		ComponentManager* mComponentManager;
		SystemManager* mSystemManager;
	};
}

#endif // __ECS_COORDINATOR_H__
