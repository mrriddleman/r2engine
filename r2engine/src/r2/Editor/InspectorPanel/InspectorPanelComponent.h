#ifdef R2_EDITOR

#ifndef __INSPECTOR_PANEL_COMPONENT_H__
#define __INSPECTOR_PANEL_COMPONENT_H__

#include <functional>

#include "r2/Game/ECS/Entity.h"

namespace r2
{
	class Editor;
}

namespace r2::ecs
{
	class ECSCoordinator;
}

namespace r2::edit
{
	class InspectorPanel;
	
}

namespace r2::edit
{
	using InspectorPanelComponentWidgetFunc = std::function<void(Editor*editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)>;
	using InspectorPanelRemoveComponentFunc = std::function<void(r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)>;
	using InspectorPanelAddComponentFunc = std::function<void(r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)>;
	using InspectorPanelAddInstanceComponentFunc = std::function<void(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)>;

	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel);

	
	class InspectorPanelComponentDataSource;

	class InspectorPanelComponentWidget
	{
	public:
		InspectorPanelComponentWidget();

		InspectorPanelComponentWidget(s32 sortOrder, std::shared_ptr<InspectorPanelComponentDataSource> componentDataSource);

		void ImGuiDraw(InspectorPanel& inspectorPanel, ecs::Entity theEntity);

		inline s32 GetSortOrder() const { return mSortOrder; }

		void AddComponentToEntity(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity);

		//bool HasAddComponentFunc() const { return mAddComponentFunc != nullptr; }
		bool CanAddComponent(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const;

		r2::ecs::ComponentType GetComponentType() const;

		u64 GetComponentTypeHash() const;

		//
		//inline InspectorPanelAddInstanceComponentFunc GetAddInstanceComponentFunc() { return mAddInstanceComponentFunc; }

	private:

		//InspectorPanelComponentWidgetFunc mComponentWidgetFunc;
		//InspectorPanelRemoveComponentFunc mRemoveComponentFunc;
		//InspectorPanelAddComponentFunc mAddComponentFunc;
		//InspectorPanelAddInstanceComponentFunc mAddInstanceComponentFunc;

		//r2::ecs::ComponentType mComponentType;
		//u64 mComponentTypeHash;
		//std::string mComponentName;

		std::shared_ptr<InspectorPanelComponentDataSource> mComponentDataSource;
		s32 mSortOrder;
	};
}

#endif
#endif
