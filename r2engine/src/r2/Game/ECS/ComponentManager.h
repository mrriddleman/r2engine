#ifndef __COMPONENT_MANAGER_H__
#define __COMPONENT_MANAGER_H__

//https://austinmorlan.com/posts/entity_component_system/#further-readings

#include <typeindex>
#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Game/ECS/ComponentArray.h"
#include "r2/Game/ECS/Entity.h"

namespace r2::ecs
{
	using ComponentType = s32;

	class ComponentManager
	{
	public:
		ComponentManager()
			: mComponentArrays(nullptr)
			, mComponentTypes(nullptr)
		{
		}

		template<class ARENA, typename Component>
		void RegisterComponentType(ARENA& arena)
		{
			auto componentTypeHash = std::type_index(typeid(Component)).hash_code();

			if (r2::shashmap::Has(*mComponentTypes, componentTypeHash))
			{
				return;
			}

			ComponentArray<Component>* componentArray = ALLOC(ComponentArray<Component>, arena);

			R2_CHECK(componentArray != nullptr, "componentArray is nullptr");

			bool isInitialized = componentArray->Init(arena, MAX_NUM_ENTITIES);

			R2_CHECK(isInitialized, "Couldn't initialize the componentArray!");

			r2::shashmap::Set(*mComponentTypes, componentTypeHash, static_cast<ComponentType>(r2::sarr::Size(*mComponentArrays)) );

			r2::sarr::Push(*mComponentArrays, (IComponentArray*)componentArray);
		}

		template<class ARENA, typename Component>
		void UnRegisterComponentType(ARENA& arena)
		{
			auto componentTypeHash = std::type_index(typeid(Component)).hash_code();

			if (!r2::shashmap::Has(*mComponentTypes, componentTypeHash))
			{
				return;
			}

			ComponentArray<Component>* componentArray = GetComponentArray<Component>();

			componentArray->Shutdown<ARENA>(arena);

			ComponentType defaultType = -1;

			ComponentType componentType = r2::shashmap::Get(*mComponentTypes, componentTypeHash, defaultType);

			R2_CHECK(defaultType != componentType, "These shouldn't be the same");

			r2::shashmap::Remove(*mComponentTypes, componentTypeHash);

			FREE(componentArray, arena);

			r2::sarr::At(*mComponentArrays, componentType) = nullptr;
		}

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumComponents)
		{
			R2_CHECK(maxNumComponents > 0, "This should be non zero");

			mComponentArrays = MAKE_SARRAY(arena, IComponentArray*, maxNumComponents);

			if (!mComponentArrays)
			{
				R2_CHECK(false, "We couldn't create the component array");
				return false;
			}
			
			mComponentTypes = MAKE_SHASHMAP(arena, ComponentType, maxNumComponents * r2::SHashMap<ComponentType>::LoadFactorMultiplier());

			if (!mComponentTypes)
			{
				R2_CHECK(false, "We couldn't create the component types!");
				return false;
			}

			return true;
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			R2_CHECK(r2::sarr::IsEmpty(*mComponentTypes->mData), "This should be empty by the time we get here: did you forget to unregister all of the components?");

			r2::sarr::Clear(*mComponentArrays);
			r2::shashmap::Clear(*mComponentTypes);

			FREE(mComponentTypes, arena);
			FREE(mComponentArrays, arena);
		}

		template<typename Component>
		ComponentType GetComponentType()
		{
			auto componentTypeHash = std::type_index(typeid(Component)).hash_code();

			R2_CHECK(r2::shashmap::Has(*mComponentTypes, componentTypeHash), "We don't have that type!");

			ComponentType defaultIndex = -1;

			ComponentType index = r2::shashmap::Get(*mComponentTypes, componentTypeHash, defaultIndex);

			R2_CHECK(index != defaultIndex, "Should not be the same!");

			return index;
		}

		template<typename Component>
		void AddComponent(Entity entity, Component component)
		{
			ComponentArray<Component>* componentArray = GetComponentArray<Component>();
			componentArray->AddComponent(entity, component);
		}

		template<typename Component>
		void RemoveComponent(Entity entity)
		{
			ComponentArray<Component>* componentArray = GetComponentArray<Component>();
			componentArray->RemoveComponent(entity);
		}

		template<typename Component>
		Component& GetComponent(Entity entity)
		{
			ComponentArray<Component>* componentArray = GetComponentArray<Component>();
			return componentArray->GetComponent(entity);
		}

		template<typename Component>
		Component* GetComponentPtr(Entity entity)
		{
			ComponentArray<Component>* componentArray = GetComponentArray<Component>();
			return componentArray->GetComponentPtr(entity);
		}

		template<typename Component>
		void SetComponent(Entity entity, const Component& component)
		{
			ComponentArray<Component>* componentArray = GetComponentArray<Component>();
			componentArray->SetComponent(entity, component);
		}

		void EntityDestroyed(Entity entity)
		{
			// Notify each component array that an entity has been destroyed
			// If it has a component for that entity, it will remove it
			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);
			
			for (size_t i = 0; i < numComponentArrays; i++)
			{
				auto const& component = r2::sarr::At(*mComponentArrays, i);

				component->EntityDestroyed(entity);
			}
		}

		static u64 MemorySize(u32 maxNumComponents, u32 alignment, u32 headerSize, u32 boundsChecking)
		{
			u64 memorySize = 0;

			memorySize +=
				r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ComponentManager), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<IComponentArray*>::MemorySize(maxNumComponents), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<ComponentType>::MemorySize(static_cast<u32>( maxNumComponents * r2::SHashMap<ComponentType>::LoadFactorMultiplier())), alignment, headerSize, boundsChecking)
				;

			return memorySize;
		}

	private:
		r2::SArray<IComponentArray*>* mComponentArrays;
		r2::SHashMap<ComponentType>* mComponentTypes; //I guess these are the indices of the component arrays?

		template<typename Component>
		ComponentArray<Component>* GetComponentArray()
		{
			ComponentType index = GetComponentType<Component>();

			IComponentArray* componentArrayI = r2::sarr::At(*mComponentArrays, index);


			return static_cast<ComponentArray<Component>*>(componentArrayI);

			//return std::static_pointer_cast<ComponentArray<Component>>(r2::sarr::At(*mComponentArrays, index));
		}
	};
}

#endif // __COMPONENT_MANAGER_H__
