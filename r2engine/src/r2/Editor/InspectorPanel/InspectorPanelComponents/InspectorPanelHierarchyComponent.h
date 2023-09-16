#ifdef R2_EDITOR
#ifndef __INSPECTOR_PANEL_HIERARCHY_COMPONENT_H__
#define __INSPECTOR_PANEL_HIERARCHY_COMPONENT_H__

#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	//void InspectorPanelHierarchyComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator);

	class InspectorPanelHierarchyComponentDataSource : public InspectorPanelComponentDataSource
	{
	public:
		InspectorPanelHierarchyComponentDataSource();
		InspectorPanelHierarchyComponentDataSource(r2::Editor* noptrEditor, ecs::ECSCoordinator* coordinator);

		virtual void DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual bool InstancesEnabled() const override;
		virtual u32 GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const override;

		virtual void* GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void* GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual void DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual void AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void AddNewInstance(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
	private:
		r2::Editor* mnoptrEditor;
	};
}

#endif
#endif