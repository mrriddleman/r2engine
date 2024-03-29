#ifndef __COMPONENT_MANAGER_H__
#define __COMPONENT_MANAGER_H__

//https://austinmorlan.com/posts/entity_component_system/#further-readings

#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include "r2/Game/ECS/ComponentArray.h"
#include "r2/Game/ECS/Entity.h"
#include "r2/Utils/Hash.h"
#include "r2/Utils/Utils.h"
#include "r2/Game/ECS/ComponentArrayData_generated.h"
#include "r2/Game/ECS/Serialization/EditorComponentSerialization.h"
#include "r2/Game/ECS/Serialization/HeirarchyComponentSerialization.h"
#include "r2/Game/ECS/Serialization/RenderComponentSerialization.h"
#include "r2/Game/ECS/Serialization/SkeletalAnimationComponentSerialization.h"
#include "r2/Game/ECS/Serialization/TransformComponentSerialization.h"
#include "r2/Game/ECS/Serialization/AudioListenerComponentSerialization.h"
#include "r2/Game/ECS/Serialization/AudioEmitterComponentSerialization.h"
#include "r2/Game/ECS/Serialization/PointLightComponentSerialization.h"
#include "r2/Game/ECS/Serialization/SpotLightComponentSerialization.h"
#include "r2/Game/ECS/Serialization/PlayerComponentSerialization.h"

namespace r2::ecs
{
	class ECSWorld;

	const u64 EMPTY_COMPONENT_HASH = STRING_ID("");

	class ComponentManager
	{
	public:
		ComponentManager()
			: mComponentArrays(nullptr)
			, mComponentTypes(nullptr)
			, mComponentHashNameMap(nullptr)
			, mFreeComponentFuncMap(nullptr)
		{
		}

		template<class ARENA, typename Component>
		void RegisterComponentType(ARENA& arena, const char* componentName, bool shouldSerialize, bool isInstanced, FreeComponentFunc freeComponentFunc)
		{
			auto componentTypeHash = STRING_ID(componentName);

			if (r2::shashmap::Has(*mComponentTypes, componentTypeHash))
			{
				R2_CHECK(false, "You're trying to register the same component twice");
				return;
			}

			ComponentArray<Component>* componentArray = ALLOC(ComponentArray<Component>, arena);

			R2_CHECK(componentArray != nullptr, "componentArray is nullptr");
			componentArray->mShouldSerialize = shouldSerialize;

			bool isInitialized = componentArray->Init(arena, MAX_NUM_ENTITIES, componentTypeHash, componentName, isInstanced);

			R2_CHECK(isInitialized, "Couldn't initialize the componentArray!");

			r2::shashmap::Set(*mComponentHashNameMap, typeid(Component).hash_code(), componentTypeHash);

			r2::shashmap::Set(*mComponentTypes, componentTypeHash, static_cast<ComponentType>(r2::sarr::Size(*mComponentArrays)) );

			r2::shashmap::Set(*mFreeComponentFuncMap, componentTypeHash, freeComponentFunc);

			r2::sarr::Push(*mComponentArrays, (IComponentArray*)componentArray);
		}

		template<class ARENA, typename Component>
		void UnRegisterComponentType(ARENA& arena)
		{
			u64 defaultComponentHashName = EMPTY_COMPONENT_HASH;

			u64 componentTypeHash = r2::shashmap::Get(*mComponentHashNameMap, typeid(Component).hash_code(), defaultComponentHashName);

			R2_CHECK(componentTypeHash != defaultComponentHashName, "Should never happen");
			
			if (!r2::shashmap::Has(*mComponentTypes, componentTypeHash))
			{
				R2_CHECK(false, "Trying to unregister a component that we never registered");
				return;
			}

			ComponentArray<Component>* componentArray = GetComponentArray<Component>();

			FreeComponentFunc defaultFreeFunc = nullptr;

			FreeComponentFunc freeFunc = r2::shashmap::Get(*mFreeComponentFuncMap, componentTypeHash, defaultFreeFunc);

			componentArray->DestoryAllEntities(freeFunc);

			componentArray->Shutdown<ARENA>(arena);

			ComponentType defaultType = -1;

			ComponentType componentType = r2::shashmap::Get(*mComponentTypes, componentTypeHash, defaultType);

			R2_CHECK(defaultType != componentType, "These shouldn't be the same");

			r2::shashmap::Remove(*mComponentTypes, componentTypeHash);

			r2::shashmap::Remove(*mComponentHashNameMap, typeid(Component).hash_code());
			
			r2::shashmap::Remove(*mFreeComponentFuncMap, componentTypeHash);

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

			mComponentHashNameMap = MAKE_SHASHMAP(arena, u64, maxNumComponents * r2::SHashMap<ComponentType>::LoadFactorMultiplier());

			if (!mComponentHashNameMap)
			{
				R2_CHECK(false, "We couldn't create the component hash name map");
				return false;
			}

			mFreeComponentFuncMap = MAKE_SHASHMAP(arena, FreeComponentFunc, maxNumComponents * r2::SHashMap<ComponentType>::LoadFactorMultiplier());

			if (!mFreeComponentFuncMap)
			{
				R2_CHECK(false, "We couldn't create the mFreeComponentFuncMap");
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
			r2::shashmap::Clear(*mComponentHashNameMap);
			r2::shashmap::Clear(*mFreeComponentFuncMap);

			FREE(mFreeComponentFuncMap, arena);
			FREE(mComponentHashNameMap, arena);
			FREE(mComponentTypes, arena);
			FREE(mComponentArrays, arena);
			
		}

		template<typename Component>
		ComponentType GetComponentType()
		{
			R2_CHECK(r2::shashmap::Has(*mComponentHashNameMap, typeid(Component).hash_code()), "We don't have that type!");

			u64 defaultComponentHashName = EMPTY_COMPONENT_HASH;

			u64 componentTypeHash = r2::shashmap::Get(*mComponentHashNameMap, typeid(Component).hash_code(), defaultComponentHashName);

			R2_CHECK(componentTypeHash != EMPTY_COMPONENT_HASH, "Should not happen");

			ComponentType defaultIndex = -1;

			ComponentType index = r2::shashmap::Get(*mComponentTypes, componentTypeHash, defaultIndex);

			R2_CHECK(index != defaultIndex, "Should not be the same!");

			return index;
		}

		ComponentType GetComponentType(u64 componentTypeHash) const
		{
			R2_CHECK(componentTypeHash != EMPTY_COMPONENT_HASH, "Should not happen");

			ComponentType defaultIndex = -1;

			ComponentType index = r2::shashmap::Get(*mComponentTypes, componentTypeHash, defaultIndex);

			R2_CHECK(index != defaultIndex, "Should not be the same!");

			return index;
		}

		template<typename Component>
		u64 GetComponentTypeHash() const
		{
			R2_CHECK(r2::shashmap::Has(*mComponentHashNameMap, typeid(Component).hash_code()), "We don't have that type!");

			u64 defaultComponentHashName = EMPTY_COMPONENT_HASH;

			u64 componentTypeHash = r2::shashmap::Get(*mComponentHashNameMap, typeid(Component).hash_code(), defaultComponentHashName);

			R2_CHECK(componentTypeHash != EMPTY_COMPONENT_HASH, "Should not happen");

			return componentTypeHash;
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

			u64 componentTypeHash = GetComponentTypeHash<Component>();

			FreeComponentFunc defaultFreeComponentFunc = nullptr;

			FreeComponentFunc freeFunc = r2::shashmap::Get(*mFreeComponentFuncMap, componentTypeHash, defaultFreeComponentFunc);

			componentArray->RemoveComponent(entity, freeFunc);
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

			u64 componentTypeHash = GetComponentTypeHash<Component>();

			FreeComponentFunc defaultFreeComponentFunc = nullptr;

			FreeComponentFunc freeFunc = r2::shashmap::Get(*mFreeComponentFuncMap, componentTypeHash, defaultFreeComponentFunc);

			componentArray->SetComponent(entity, component, freeFunc);
		}

		void EntityDestroyed(Entity entity)
		{
			// Notify each component array that an entity has been destroyed
			// If it has a component for that entity, it will remove it
			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);
			
			for (size_t i = 0; i < numComponentArrays; i++)
			{
				auto const& componentArray = r2::sarr::At(*mComponentArrays, i);
				auto componentTypeHash = componentArray->GetHashName();

				FreeComponentFunc defaultFreeComponentFunc = nullptr;

				FreeComponentFunc freeFunc = r2::shashmap::Get(*mFreeComponentFuncMap, componentTypeHash, defaultFreeComponentFunc);

				componentArray->EntityDestroyed(entity, freeFunc);
			}
		}

		void DestroyAllEntities()
		{
			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);

			for (size_t i = 0; i < numComponentArrays; i++)
			{
				auto const& componentArray = r2::sarr::At(*mComponentArrays, i);

				auto componentTypeHash = componentArray->GetHashName();

				FreeComponentFunc defaultFreeComponentFunc = nullptr;

				FreeComponentFunc freeFunc = r2::shashmap::Get(*mFreeComponentFuncMap, componentTypeHash, defaultFreeComponentFunc);

				componentArray->DestoryAllEntities(freeFunc);
			}
		}

		void Serialize(flatbuffers::FlatBufferBuilder& builder, std::vector<flatbuffers::Offset<flat::ComponentArrayData>>& componentArraysData) const
		{
			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);
			componentArraysData.reserve(numComponentArrays);

			for (u32 i = 0; i < numComponentArrays; ++i)
			{
				IComponentArray* componentArray = r2::sarr::At(*mComponentArrays, i);

				if (componentArray->mShouldSerialize)
				{
					componentArraysData.push_back(componentArray->Serialize(builder));
				}
			}
		}

		void DeSerialize(ECSWorld& ecsWorld, const r2::SArray<ecs::Entity>* ecsEntitiesToPopulate, const r2::SArray<const flat::EntityData*>* refEntityData, r2::SArray<Signature>* entitySignatures, const flatbuffers::Vector<flatbuffers::Offset<flat::ComponentArrayData>>* componentArrays)
		{
			//We need the mComponentArrays to exist already because we have no reflection in order to load the component arrays from flatbuffers
			R2_CHECK(componentArrays->size() <= r2::sarr::Size(*mComponentArrays), "Right now these should be the same or the flatbuffer data should have less (since we don't serialize all of them) - ie. mComponentArrays should exist already");

			for (flatbuffers::uoffset_t i = 0; i < componentArrays->size(); ++i)
			{
				const auto& componentArray = componentArrays->Get(i);

				const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);

				IComponentArray* componentArrayToLoad = nullptr;

				for (u32 j = 0; j < numComponentArrays; ++j)
				{
					IComponentArray* icomponentArray = r2::sarr::At(*mComponentArrays, j);

					if (icomponentArray->GetHashName() == componentArray->componentType())
					{
						componentArrayToLoad = icomponentArray;
						break;
					}
				}

				R2_CHECK(componentArrayToLoad != nullptr, "this should not be null!");

				ComponentType defaultIndex = -1;
				ComponentType componentTypeIndex = r2::shashmap::Get(*mComponentTypes, componentArrayToLoad->GetHashName(), defaultIndex);
				
				R2_CHECK(componentTypeIndex != defaultIndex, "Should never happen");

				componentArrayToLoad->DeSerializeForEntities(ecsWorld, ecsEntitiesToPopulate, refEntityData, entitySignatures, componentTypeIndex, componentArray);
			}
		}

		static u64 MemorySize(u32 maxNumComponents, u32 alignment, u32 headerSize, u32 boundsChecking)
		{
			u64 memorySize = 0;

			memorySize +=
				r2::mem::utils::GetMaxMemoryForAllocation(sizeof(ComponentManager), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<IComponentArray*>::MemorySize(maxNumComponents), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<ComponentType>::MemorySize(static_cast<u32>(maxNumComponents * r2::SHashMap<ComponentType>::LoadFactorMultiplier())), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<u64>::MemorySize(static_cast<u32>(maxNumComponents * r2::SHashMap<ComponentType>::LoadFactorMultiplier())), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<FreeComponentFunc>::MemorySize(static_cast<u32>(maxNumComponents * r2::SHashMap<u32>::LoadFactorMultiplier())), alignment, headerSize, boundsChecking)
				;
			return memorySize;
		}

#ifdef R2_EDITOR
		std::vector<std::string> GetAllRegisteredComponentNames() const
		{
			std::vector<std::string> componentNames;

			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);

			for (u32 i = 0; i < numComponentArrays; ++i)
			{
				componentNames.push_back(r2::sarr::At(*mComponentArrays, i)->GetComponentName());
			}

			return componentNames;
		}

		std::vector<u64> GetAllRegisteredComponentTypeHashes() const
		{
			std::vector<u64> componentTypeHashes;

			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);

			for (u32 i = 0; i < numComponentArrays; ++i)
			{
				componentTypeHashes.push_back(r2::sarr::At(*mComponentArrays, i)->GetHashName());
			}

			return componentTypeHashes;
		}

		std::vector<std::string> GetAllRegisteredNonInstancedComponentNames() const
		{
			std::vector<std::string> componentNames;

			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);

			for (u32 i = 0; i < numComponentArrays; ++i)
			{
				IComponentArray* componentArray = r2::sarr::At(*mComponentArrays, i);

				if (!componentArray->IsInstanced())
				{
					componentNames.push_back(componentArray->GetComponentName());
				}
			}

			return componentNames;
		}

		std::vector<u64> GetAllRegisteredNonInstancedComponentTypeHashes() const
		{
			std::vector<u64> componentTypeHashes;

			const auto numComponentArrays = r2::sarr::Size(*mComponentArrays);

			for (u32 i = 0; i < numComponentArrays; ++i)
			{
				IComponentArray* componentArray = r2::sarr::At(*mComponentArrays, i);

				if (!componentArray->IsInstanced())
				{
					componentTypeHashes.push_back(componentArray->GetHashName());
				}
			}

			return componentTypeHashes;
		}

#endif
		template<typename Component>
		u64 GetComponentTypeHash()
		{
			u64 defaultComponentHashName = EMPTY_COMPONENT_HASH;

			u64 componentTypeHash = r2::shashmap::Get(*mComponentHashNameMap, typeid(Component).hash_code(), defaultComponentHashName);

			R2_CHECK(componentTypeHash != defaultComponentHashName, "Should never happen");

			if (!r2::shashmap::Has(*mComponentTypes, componentTypeHash))
			{
				R2_CHECK(false, "Trying to get a component type hash that we don't have?");
				return 0;
			}

			return componentTypeHash;
		}
	private:
		r2::SArray<IComponentArray*>* mComponentArrays;
		r2::SHashMap<ComponentType>* mComponentTypes; //I guess these are the indices of the component arrays?
		r2::SHashMap<u64>* mComponentHashNameMap;
		r2::SHashMap<FreeComponentFunc>* mFreeComponentFuncMap;

		template<typename Component>
		ComponentArray<Component>* GetComponentArray()
		{
			ComponentType index = GetComponentType<Component>();

			IComponentArray* componentArrayI = r2::sarr::At(*mComponentArrays, index);

			return static_cast<ComponentArray<Component>*>(componentArrayI);
		}
	};
}

#endif // __COMPONENT_MANAGER_H__
