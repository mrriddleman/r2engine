#ifndef __FACING_COMPONENT_INSPECTOR_PANEL_WIDGET_H__
#define __FACING_COMPONENT_INSPECTOR_PANEL_WIDGET_H__

#ifdef R2_EDITOR

#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"
#include "r2/Utils/Utils.h"

namespace r2
{
	class Editor;
}

namespace r2::ecs 
{
	class InspectorPanel;
}

class InspectorPanelFacingDataSource : public r2::edit::InspectorPanelComponentDataSource
{
public:
	InspectorPanelFacingDataSource();
	InspectorPanelFacingDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const r2::edit::InspectorPanel* inspectorPanel);

	virtual void DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) override;

	virtual bool InstancesEnabled() const override;
	virtual u32 GetNumInstances(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) const override;

	virtual void* GetComponentData(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) override;
	virtual void* GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) override;

	virtual void DeleteComponent(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) override;
	virtual void DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) override;

	virtual void AddComponent(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) override;
	virtual void AddNewInstances(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity, u32 numInstances) override;
private:
	r2::Editor* mnoptrEditor;
	const r2::edit::InspectorPanel* mInspectorPanel;
};

#endif
#endif