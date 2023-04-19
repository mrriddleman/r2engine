#ifndef __COMPONENT_ARRAY_H__
#define __COMPONENT_ARRAY_H__

//https://austinmorlan.com/posts/entity_component_system/#further-readings

#include "r2/Game/ECS/Entity.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Game/ECS/ComponentArrayData_generated.h"

#include "r2/Game/ECS/Serialization/ComponentArraySerialization.h"

namespace r2::ecs
{

	//https://stackoverflow.com/questions/14727313/c-how-to-reference-templated-functions-using-stdbind-stdfunction
	//class ComponentArrayHydration
	//{
	//public:
	//	template <typename Component>
	//	struct HydrateFunction
	//	{
	//		static std::function<void(r2::SArray<Component>&)> Function = nullptr;
	//	};
	//};

	//template<typename Component>
	//std::function<void(r2::SArray<Component>&)> ComponentArrayHydration::HydrateFunction<Component>::Function = nullptr;

	using ComponentType = s32;
	using ComponentArrayHydrationFunction = std::function<void*(void*)>;

	class IComponentArray
	{
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
		virtual void DestoryAllEntities() = 0;
		virtual u64 GetHashName() = 0;
		virtual flatbuffers::Offset<flat::ComponentArrayData> Serialize(flatbuffers::FlatBufferBuilder& builder) const = 0;
		virtual void DeSerializeForEntities(
			const r2::SArray<Entity>* entitiesToAddComponentsTo,
			const r2::SArray<const flat::EntityData*>* refEntityData,
			r2::SArray<Signature>* entitySignatures,
			ComponentType componentType,
			const flat::ComponentArrayData* componentArrayData,
			ComponentArrayHydrationFunction hydrateFunction) = 0;

		b32 mShouldSerialize;
	};

	template <typename Component>
	class ComponentArray : public IComponentArray
	{
	public:

		ComponentArray()
			: mHashName (0)
			, mComponentArray(nullptr)
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
		bool Init(ARENA& arena, u32 maxNumEntities, u64 hashName)
		{
			mHashName = hashName;

			mComponentArray = MAKE_SARRAY(arena, Component, maxNumEntities);

			if (mComponentArray == nullptr)
			{
				R2_CHECK(false, "Failed to create the component array!");
				return false;
			}

			mEntityToIndexMap = MAKE_SARRAY(arena, s32, maxNumEntities+1);
			if (!mEntityToIndexMap)
			{
				R2_CHECK(false, "Failed to create mEntityToIndexMap!");
				return false;
			}

			r2::sarr::Fill(*mEntityToIndexMap, static_cast<s32>(-1));

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
			R2_CHECK(r2::sarr::At(*mEntityToIndexMap, entity) == -1, "We already have a component associated with this entity");

			const auto index = static_cast<s32>(r2::sarr::Size(*mComponentArray));
			r2::sarr::Push(*mComponentArray, component);

			r2::sarr::At(*mEntityToIndexMap, entity) = index;

			r2::shashmap::Set(*mIndexToEntityMap, index, entity);
		}

		void RemoveComponent(Entity entity)
		{
			R2_CHECK(r2::sarr::At(*mEntityToIndexMap, entity) != -1, "We should already have this entity in the component array");

			s32 defaultIndex = -1;
			auto indexOfRemovedEntity = r2::sarr::At(*mEntityToIndexMap, entity);
			R2_CHECK(indexOfRemovedEntity != defaultIndex, "This must exist!");

			s32 indexOfLastElement = r2::sarr::Size(*mComponentArray) - 1;

			const Entity& entityOfLastElement = r2::shashmap::Get(*mIndexToEntityMap, indexOfLastElement, INVALID_ENTITY);
			
			R2_CHECK(entityOfLastElement != INVALID_ENTITY, "Should not be invalid");

			r2::sarr::RemoveAndSwapWithLastElement(*mComponentArray, indexOfRemovedEntity);
			
			r2::sarr::At(*mEntityToIndexMap, entityOfLastElement) = indexOfRemovedEntity;
			r2::shashmap::Set(*mIndexToEntityMap, indexOfRemovedEntity, entityOfLastElement);

			r2:sarr::At(*mEntityToIndexMap, entity) = -1;
			r2::shashmap::Remove(*mIndexToEntityMap, indexOfLastElement);
		}

		Component& GetComponent(Entity entity)
		{
			R2_CHECK(r2::sarr::At(*mEntityToIndexMap, entity) != -1, "We should already have this entity in the component array");

			const s32 defaultIndex = -1;
			s32 index = r2::sarr::At(*mEntityToIndexMap, entity);
			R2_CHECK(index != defaultIndex, "Should never happen!");

			return r2::sarr::At(*mComponentArray, index);
		}

		Component* GetComponentPtr(Entity entity)
		{
			const s32 defaultIndex = -1;
			s32 index = r2::sarr::At(*mEntityToIndexMap, entity);

			if (defaultIndex == index)
			{
				return nullptr;
			}

			return &r2::sarr::At(*mComponentArray, index);
		}

		void SetComponent(Entity entity, const Component& component)
		{
			R2_CHECK(r2::sarr::At(*mEntityToIndexMap, entity) != -1, "We should already have this entity in the component array");

			const s32 defaultIndex = -1;
			s32 index = r2::sarr::At(*mEntityToIndexMap, entity);
			R2_CHECK(index != defaultIndex, "Should never happen!");


			r2::sarr::At(*mComponentArray, index) = component;
		}

		void EntityDestroyed(Entity entity) override
		{
			if (r2::sarr::At(*mEntityToIndexMap, entity) != -1)
			{
				RemoveComponent(entity);
			}
		}

		void DestoryAllEntities() override
		{
			r2::sarr::Clear(*mComponentArray);
			r2::sarr::Clear(*mEntityToIndexMap);
			r2::sarr::Fill(*mEntityToIndexMap, static_cast<s32>(-1));
			r2::shashmap::Clear(*mIndexToEntityMap);
		}

		flatbuffers::Offset<flat::ComponentArrayData> Serialize(flatbuffers::FlatBufferBuilder& builder) const override
		{
			flexbuffers::Builder flexbufferBuilder;
			SerializeComponentArray(flexbufferBuilder, *mComponentArray);
			flexbufferBuilder.Finish();

			auto flexBufferData = builder.CreateVector(flexbufferBuilder.GetBuffer());
			std::vector<flatbuffers::Offset<flat::EntityToIndexMapEntry>> entityToIndexEntries;

			for (u32 i = 0; i < r2::sarr::Capacity(*mEntityToIndexMap); ++i)
			{
				auto index = r2::sarr::At(*mEntityToIndexMap, i);
				if (index != -1)
				{
					auto entry = flat::CreateEntityToIndexMapEntry(builder, i, index);
					entityToIndexEntries.push_back(entry);
				}
			}

			auto entityToIndexMapVec = builder.CreateVector(entityToIndexEntries);

			flat::ComponentArrayDataBuilder componentArrayDataBuilder(builder);
			componentArrayDataBuilder.add_componentType(mHashName);
			componentArrayDataBuilder.add_componentArray(flexBufferData);
			componentArrayDataBuilder.add_entityToIndexMap(entityToIndexMapVec);

			return componentArrayDataBuilder.Finish();
		}

		void DeSerializeForEntities(
			const r2::SArray<Entity>* entitiesToAddComponentsTo,
			const r2::SArray<const flat::EntityData*>* refEntityData,
			r2::SArray<Signature>* entitySignatures,
			ComponentType componentType,
			const flat::ComponentArrayData* componentArrayData,
			ComponentArrayHydrationFunction hydrateFunction) override
		{
			R2_CHECK(componentArrayData != nullptr, "Can't be nullptr");
			R2_CHECK(entitiesToAddComponentsTo != nullptr, "Can't be nullptr");
			R2_CHECK(refEntityData != nullptr, "Can't be nullptr");
			const auto numEntitiesToAddComponentsTo = r2::sarr::Size(*entitiesToAddComponentsTo);
			const auto numRefEntities = r2::sarr::Size(*refEntityData);

			R2_CHECK(numEntitiesToAddComponentsTo == numRefEntities, "These should be the same");

			r2::SArray<Component>* tempComponents = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, Component, MAX_NUM_ENTITIES);

			mHashName = componentArrayData->componentType();

			DeSerializeComponentArray(*tempComponents, entitiesToAddComponentsTo, refEntityData, componentArrayData);

			r2::SArray<Component>* realComponents = tempComponents;

			if (hydrateFunction)
			{
				realComponents = static_cast<r2::SArray<Component>*>( hydrateFunction(static_cast<void*>(tempComponents)) );
			}

			const auto* entityToIndexMap = componentArrayData->entityToIndexMap();

			for (u32 i = 0; i < numRefEntities; ++i)
			{
				const flat::EntityData* entityData = r2::sarr::At(*refEntityData, i);

				s32 componentIndex = -1;

				for (flatbuffers::uoffset_t j = 0; j < entityToIndexMap->size(); ++j)
				{
					const auto* entry = entityToIndexMap->Get(j);

					if (entry->entity() == entityData->entityID())
					{
						componentIndex = entry->index();
						break;
					}
				}
				
				R2_CHECK(componentIndex != -1, "Should never happen");

				Entity e = r2::sarr::At(*entitiesToAddComponentsTo, i);

				const auto& component = r2::sarr::At(*realComponents, componentIndex);

				AddComponent(e, component);

				auto& signature = r2::sarr::At(*entitySignatures, i);

				signature.set(componentType, true);
			}
			

			CleanupDeserializeComponentArray(*tempComponents);

			FREE(tempComponents, *MEM_ENG_SCRATCH_PTR);
		}

		u64 GetHashName() override
		{
			return mHashName;
		}

		static u64 MemorySize(u32 maxNumEntities, u64 alignment, u32 headerSize, u32 boundsChecking)
		{
			u64 memorySize = 0;

			memorySize += 
				r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ComponentArray<Component>), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Component>::MemorySize(maxNumEntities), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(maxNumEntities+1), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<Entity>::MemorySize(maxNumEntities * r2::SHashMap<u32>::LoadFactorMultiplier()), alignment, headerSize, boundsChecking);

			return memorySize;
		}

	private:
		u64 mHashName;
		r2::SArray<Component>* mComponentArray;
		r2::SArray<s32>* mEntityToIndexMap;
		r2::SHashMap<Entity>* mIndexToEntityMap;
	};
}

#endif // __COMPONENT_ARRAY_H__
