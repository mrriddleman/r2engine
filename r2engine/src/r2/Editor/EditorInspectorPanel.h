
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

		//@TODO(Serge): add in the component type hash
		void RegisterComponentType(
			const std::string& componentName,
			u32 sortOrder,
			r2::ecs::ComponentType componentType,
			u64 componentTypeHash,
			InspectorPanelComponentWidgetFunc componentWidget,
			InspectorPanelRemoveComponentFunc removeComponentFunc,
			InspectorPanelAddComponentFunc addComponentFunc);

		Editor* GetEditor();

	private:
		std::vector<InspectorPanelComponentWidget> mComponentWidgets;
		ecs::Entity mEntitySelected;
		s32 mCurrentComponentIndexToAdd;
	};
}

#endif // __EDITOR_INSPECTOR_PANEL_H__
#endif