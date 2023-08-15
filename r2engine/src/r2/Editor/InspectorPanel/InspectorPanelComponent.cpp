#include "r2pch.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"

#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
//ImGui Components
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"

namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::EditorComponent>(), InspectorPanelEditorComponent);
		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::TransformComponent>(), InspectorPanelTransformComponent);
		//inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(), InspectorPanelInstancedTransformComponent);
	}
}