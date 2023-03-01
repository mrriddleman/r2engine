#ifndef __COMPONENT_ARRAY_H__
#define __COMPONENT_ARRAY_H__

//https://austinmorlan.com/posts/entity_component_system/#further-readings

#include "r2/Game/ECS/Entity.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::ecs
{
	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
	};


	template <typename Component>
	class ComponentArray : public IComponentArray
	{
	public:

		ComponentArray()
			: mComponentArray(nullptr)
			, mEntityToIndexMap(nullptr)
			, mIndexToEntityMap(nullptr)
		{
		}

		~ComponentArray()
		{
			R2_CHECK(mComponentArray == nullptr, "Should be null already");
			R2_CHECK(mEntityToIndexMap == nullptr, "Should be null already");
			R2_CHECK(mIndexToEntityMap == nullptr, "Should be null already");
		}

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumEntities)
		{
			mComponentArray = MAKE_SARRAY(arena, Component, maxNumEntities);

			if (mComponentArray == nullptr)
			{
				R2_CHECK(false, "Failed to create the component array!");
				return false;
			}

			mEntityToIndexMap = MAKE_SHASHMAP(arena, s64, maxNumEntities * r2::SHashMap<u32>::LoadFactorMultiplier());
			if (!mEntityToIndexMap)
			{
				R2_CHECK(false, "Failed to create mEntityToIndexMap!");
				return false;
			}

			mIndexToEntityMap = MAKE_SHASHMAP(arena, Entity, MAX_NUM_ENTITIES * r2::SHashMap<Entity>::LoadFactorMultiplier());
			if (!mIndexToEntityMap)
			{
				R2_CHECK(false, "Failed to create mIndexToEntityMap");
				return false;
			}

			return true;
		}

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			FREE(mIndexToEntityMap, arena);
			FREE(mEntityToIndexMap, arena);
			FREE(mComponentArray, arena);

			mComponentArray = nullptr;
			mEntityToIndexMap = nullptr;
			mIndexToEntityMap = nullptr;
		}

		void AddComponent(Entity entity, Component component)
		{
			R2_CHECK(mComponentArray != nullptr, "ComponentArray not initialized!");
			R2_CHECK(mEntityToIndexMap != nullptr, "ComponentArray not initialized!");
			R2_CHECK(mIndexToEntityMap != nullptr, "ComponentArray not initialized!");

			//first check to see if this entity already exists in this component array
			R2_CHECK(!r2::shashmap::Has(*mEntityToIndexMap, entity), "We already have a component associated with this entity");

			const auto index = r2::sarr::Size(*mComponentArray);
			r2::sarr::Push(*mComponentArray, component);
			r2::shashmap::Set(*mEntityToIndexMap, entity, index);
			r2::shashmap::Set(*mIndexToEntityMap, index, entity);
		}

		void RemoveComponent(Entity entity)
		{
			R2_CHECK(r2::shashmap::Has(*mEntityToIndexMap, entity), "We should already have this entity in the component array");

			s64 defaultIndex = -1;
			auto indexOfRemovedEntity = r2::shashmap::Get(*mEntityToIndexMap, entity, defaultIndex);
			R2_CHECK(indexOfRemovedEntity != defaultIndex, "This must exist!");

			s32 indexOfLastElement = r2::sarr::Size(*mComponentArray) - 1;
			const Entity& entityOfLastElement = r2::sarr::At(*mComponentArray, indexOfLastElement);
			
			r2::sarr::RemoveAndSwapWithLastElement(*mComponentArray, indexOfRemovedEntity);
			
			r2::shashmap::Set(*mEntityToIndexMap, entityOfLastElement, indexOfRemovedEntity);
			r2::shashmap::Set(*mIndexToEntityMap, indexOfRemovedEntity, entityOfLastElement);

			r2::shashmap::Remove(*mEntityToIndexMap, entity);
			r2::shashmap::Remove(*mIndexToEntityMap, indexOfLastElement);
		}

		Component& GetComponent(Entity entity)
		{
			R2_CHECK(r2::shashmap::Has(*mEntityToIndexMap, entity), "We should already have this entity in the component array");

			s64 defaultIndex = -1;
			s64 index = r2::shashmap::Get(*mEntityToIndexMap, entity, defaultIndex);
			R2_CHECK(index != defaultIndex, "Should never happen!");

			return r2::sarr::At(*mComponentArray, index);
		}

		const Component& GetComponent(Entity entity)
		{
			return GetComponent(entity);
		}

		void EntityDestroyed(Entity entity) override
		{
			if (r2::shashmap::Has(*mEntityToIndexMap, entity))
			{
				RemoveComponent(entity);
			}
		}

		static u64 MemorySize(u32 maxNumEntities, u64 alignment, u32 headerSize, u32 boundsChecking)
		{
			u64 memorySize = 0;

			memorySize += 
				r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ComponentArray<Component>), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Component>::MemorySize(maxNumEntities), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<u32>::MemorySize(maxNumEntities * r2::SHashMap<u32>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Entity>::MemorySize(maxNumEntities * r2::SHashMap<u32>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking);

			return memorySize;
		}

	private:
		r2::SArray<Component>* mComponentArray;
		r2::SHashMap<s64>* mEntityToIndexMap;
		r2::SHashMap<Entity>* mIndexToEntityMap;
	};
}

#endif // __COMPONENT_ARRAY_H__
