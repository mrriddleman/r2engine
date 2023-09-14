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

	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel);

	class InspectorPanelComponentWidget
	{
	public:
		InspectorPanelComponentWidget();
		InspectorPanelComponentWidget(
			const std::string& componentName,
			r2::ecs::ComponentType componentType,
			u64 componentTypeHash,
			InspectorPanelComponentWidgetFunc widgetFunction,
			InspectorPanelRemoveComponentFunc removeComponentFunc,
			InspectorPanelAddComponentFunc addComponentFunc);

		void ImGuiDraw(InspectorPanel& inspectorPanel, ecs::Entity theEntity);
		inline u32 GetSortOrder() const { return mSortOrder; }
		void SetSortOrder(u32 sortOrder);
		void AddComponentToEntity(ecs::ECSCoordinator* coordinator, ecs::Entity theEntity);

		bool HasAddComponentFunc() const { return mAddComponentFunc != nullptr; }
		inline r2::ecs::ComponentType GetComponentType() const { return mComponentType; }
		inline u64 GetComponentTypeHash() const { return mComponentTypeHash; }
	private:

		InspectorPanelComponentWidgetFunc mComponentWidgetFunc;
		InspectorPanelRemoveComponentFunc mRemoveComponentFunc;
		InspectorPanelAddComponentFunc mAddComponentFunc;
		r2::ecs::ComponentType mComponentType;
		u64 mComponentTypeHash;
		std::string mComponentName;
		u32 mSortOrder;
	};
}

#endif
#endif
