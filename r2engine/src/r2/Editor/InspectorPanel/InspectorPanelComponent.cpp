#include "r2pch.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"

//ImGui Components
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"


namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::EditorComponent>(), InspectorPanelEditorComponent);
	}
}