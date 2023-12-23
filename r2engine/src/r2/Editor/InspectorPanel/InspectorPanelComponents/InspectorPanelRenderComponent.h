#ifdef R2_EDITOR
#ifndef __INSPECTOR_PANEL_RENDER_COMPONENT_H__
#define __INSPECTOR_PANEL_RENDER_COMPONENT_H__

#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"

namespace r2
{
	class Editor;
}

namespace r2::edit
{
	class InspectorPanelRenderComponentDataSource : public InspectorPanelComponentDataSource
	{
	public:
		InspectorPanelRenderComponentDataSource();
		InspectorPanelRenderComponentDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator);

		virtual void DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual bool InstancesEnabled() const override;
		virtual u32 GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const override;

		virtual void* GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void* GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual void DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;

		virtual void AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) override;
		virtual void AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances) override;

	private:

		
		r2::Editor* mnoptrEditor;

		bool mOpenMaterialsWindow;
		r2::mat::MaterialName mMaterialToEdit;
	};
}

#endif
#endif