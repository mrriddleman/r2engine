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
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel);

	class InspectorPanelComponentWidget
	{
	public:
		InspectorPanelComponentWidget();
		InspectorPanelComponentWidget(
			const std::string& componentName,
			r2::ecs::ComponentType componentType,
			InspectorPanelComponentWidgetFunc widgetFunction,
			InspectorPanelRemoveComponentFunc removeComponentFunc);

		void ImGuiDraw(InspectorPanel& inspectorPanel, ecs::Entity theEntity);
		inline u32 GetSortOrder() const { return mSortOrder; }
		void SetSortOrder(u32 sortOrder);
		inline r2::ecs::ComponentType GetComponentType() const { return mComponentType; }
	private:

		InspectorPanelComponentWidgetFunc mComponentWidgetFunc;
		InspectorPanelRemoveComponentFunc mRemoveComponentFunc;
		r2::ecs::ComponentType mComponentType;
		std::string mComponentName;
		u32 mSortOrder;
	};
}

#endif
#endif
