
#ifdef R2_EDITOR
#ifndef __EDITOR_INSPECTOR_PANEL_H__
#define __EDITOR_INSPECTOR_PANEL_H__

#include "r2/Editor/EditorWidget.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Game/ECS/Component.h"
#include "r2/Game/ECS/Entity.h"
#include <unordered_map>


namespace r2::ecs
{
	struct TransformComponent;
}

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

		void RegisterComponentWidget(const InspectorPanelComponentWidget& inspectorPanelComponentWidget);

		Editor* GetEditor();

	private:

		InspectorPanelComponentWidget* GetComponentWidgetForComponentTypeHash(u64 componentTypeHash);

		void ManipulateTransformComponent(r2::ecs::TransformComponent& transformComponent);

		std::vector<InspectorPanelComponentWidget> mComponentWidgets;
		ecs::Entity mSelectedEntity;
		s32 mCurrentComponentIndexToAdd;

		u32 mCurrentOperation;
		s32 mCurrentMode;
		s32 mCurrentInstance;
	};
}

#endif // __EDITOR_INSPECTOR_PANEL_H__
#endif