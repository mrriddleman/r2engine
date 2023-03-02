#ifndef __ECS_COORDINATOR_H__
#define __ECS_COORDINATOR_H__

//https://austinmorlan.com/posts/entity_component_system/#further-reading

#include "r2/Game/ECS/Entity.h"
#include "r2/Game/ECS/ComponentManager.h"
#include "r2/Game/ECS/SystemManager.h"

namespace r2::ecs
{
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

		template<class ARENA, typename Component>
		void RegisterComponent(ARENA& arena)
		{
			mComponentManager->RegisterComponentType<ARENA, Component>(arena);
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
		Component& GetComponent(Entity entity)
		{
			return mComponentManager->GetComponent<Component>(entity);
		}

		template<typename Component>
		ComponentType GetComponentType()
		{
			return mComponentManager->GetComponentType<Component>();
		}

		template<class ARENA, typename SystemType>
		SystemType* RegisterSystem(ARENA& arena)
		{
			return mSystemManager->RegisterSystem<ARENA, SystemType>(arena);
		}

		template<typename SystemType>
		void SetSystemSignature(Signature signature)
		{
			mSystemManager->SetSignature<SystemType>(signature);
		}

		static u64 MemorySize(u32 maxNumComponents, u32 maxNumEntities, u32 maxNumSystems, u64 alignment, u32 headerSize, u32 boundsChecking);
	private:

		EntityManager* mEntityManager;
		ComponentManager* mComponentManager;
		SystemManager* mSystemManager;
	};
}

#endif // __ECS_COORDINATOR_H__
