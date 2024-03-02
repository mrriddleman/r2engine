#ifdef R2_EDITOR
#ifndef __INSPECTOR_PANEL_SKELETAL_ANIMATION_COMPONENT_H__
#define __INSPECTOR_PANEL_SKELETAL_ANIMATION_COMPONENT_H__

#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"

namespace r2::edit
{
	//void InspectorPanelSkeletalAnimationComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator);

	class InspectorPanelSkeletonAnimationComponentDataSource : public InspectorPanelComponentDataSource
	{
	public:
		InspectorPanelSkeletonAnimationComponentDataSource();
		InspectorPanelSkeletonAnimationComponentDataSource(r2::ecs::ECSCoordinator* coordinator);

		virtual void DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance) override;

		virtual bool InstancesEnabled() const override;
		virtual u32 GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const override;

		virtual void* GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void* GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual void DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual void AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances) override;

		virtual bool CanAddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual bool ShouldDisableRemoveComponentButton() const override;
	};
}

#endif
#endif