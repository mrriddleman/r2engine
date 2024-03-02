#ifndef __INSPECTOR_PANEL_COMPONENT_DATA_SOURCE_H__
#define __INSPECTOR_PANEL_COMPONENT_DATA_SOURCE_H__

#ifdef R2_EDITOR
#include "r2/Game/ECS/Entity.h"
#include "r2/Game/ECS/Component.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"

#include "r2/Core/Engine.h"

namespace r2
{
	class Editor;
}


namespace r2::edit
{
	//Helper function for adding new instances
	template<typename T>
	inline ecs::InstanceComponentT<T>* AddNewInstanceCapacity(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{
		const bool hasInstancedComponent = coordinator->HasComponent<r2::ecs::InstanceComponentT<T>>(theEntity);

		r2::ecs::InstanceComponentT<T>* instancedComponentToUse = nullptr;

		ecs::ECSWorld& ecsWorld = MENG.GetECSWorld();

		if (!hasInstancedComponent)
		{
			r2::ecs::InstanceComponentT<T> instancedComponent;
			instancedComponent.numInstances = 0;
			instancedComponent.instances = ECS_WORLD_MAKE_SARRAY(ecsWorld, T, numInstances);

			coordinator->AddComponent<r2::ecs::InstanceComponentT<T>>(theEntity, instancedComponent);

			instancedComponentToUse = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<T>>(theEntity);
		}
		else
		{
			r2::ecs::InstanceComponentT<T>* tempInstancedComponents = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<T>>(theEntity);

			R2_CHECK(tempInstancedComponents != nullptr, "This should never happen");

			if (!(r2::sarr::Size(*tempInstancedComponents->instances) + numInstances <= r2::sarr::Capacity(*tempInstancedComponents->instances)))
			{
				r2::SArray<T>* components = ECS_WORLD_MAKE_SARRAY(ecsWorld, T, tempInstancedComponents->numInstances + numInstances);

				r2::sarr::Copy(*components, *tempInstancedComponents->instances);

				ECS_WORLD_FREE(ecsWorld, tempInstancedComponents->instances);

				tempInstancedComponents->instances = components;
			}

			instancedComponentToUse = tempInstancedComponents;
		}

		R2_CHECK(instancedComponentToUse != nullptr, "Should never happen");

		return instancedComponentToUse;
	}

	class InspectorPanelComponentDataSource
	{
	public:
		InspectorPanelComponentDataSource(const std::string& componentName, ecs::ComponentType componentType, u64 componentTypeHash)
			:mComponentName(componentName)
			, mComponentType(componentType)
			, mComponentTypeHash(componentTypeHash)
		{}

		virtual void DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance) = 0;

		virtual bool InstancesEnabled() const = 0;
		virtual u32 GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const = 0;

		virtual void* GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) = 0;
		virtual void* GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) = 0;

		virtual bool ShouldDisableRemoveComponentButton() const { return false; }

		virtual void DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) = 0;
		virtual void DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) = 0;

		virtual bool CanAddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) {return true;}

		virtual void AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) = 0;
		virtual void AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances) = 0;

		inline ecs::ComponentType GetComponentType() const { return mComponentType; }
		inline u64 GetComponentTypeHash() const { return mComponentTypeHash; }
		inline std::string GetComponentName() const { return mComponentName; }

	protected:
		bool IsInLevelMaterialList(r2::Editor* editor, const r2::asset::AssetName& materialAsset) const;

		std::string mComponentName;
		r2::ecs::ComponentType mComponentType;
		u64 mComponentTypeHash;
	};
}


#endif
#endif