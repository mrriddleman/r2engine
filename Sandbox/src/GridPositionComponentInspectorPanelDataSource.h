#ifndef __GRID_POSITION_COMPONENT_INSPECTOR_PANEL_DATA_SOURCE_H__
#define __GRID_POSITION_COMPONENT_INSPECTOR_PANEL_DATA_SOURCE_H__

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

class InspectorPanelGridPositionDataSource : public r2::edit::InspectorPanelComponentDataSource
{
public:
	InspectorPanelGridPositionDataSource();
	InspectorPanelGridPositionDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const r2::edit::InspectorPanel* inspectorPanel);

	virtual void DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity, s32 instance) override;

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

#endif // __GRID_POSITION_COMPONENT_INSPECTOR_PANEL_DATA_SOURCE_H__
