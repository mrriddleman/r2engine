
#ifdef R2_EDITOR
#ifndef __EDITOR_INSPECTOR_PANEL_H__
#define __EDITOR_INSPECTOR_PANEL_H__

#include "r2/Editor/EditorWidget.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Game/ECS/Component.h"
#include "r2/Game/ECS/Entity.h"
#include <unordered_map>

namespace r2::edit
{
	class InspectorPanel : public EditorWidget
	{
	public:
		InspectorPanel();
		~InspectorPanel();
		virtual void Init(Editor* noptrEditor) override;
		virtual void Shutdown() override;
		virtual void OnEvent(evt::Event& e) override;
		virtual void Update() override;
		virtual void Render(u32 dockingSpaceID) override;

		void RegisterComponentType(r2::ecs::ComponentType componentType, InspectorPanelComponentWidgetFunc componentWidget);

	private:
		std::unordered_map<r2::ecs::ComponentType, InspectorPanelComponentWidgetFunc> mComponentWidgets;
		ecs::Entity mEntitySelected;
	};
}

#endif // __EDITOR_INSPECTOR_PANEL_H__
#endif