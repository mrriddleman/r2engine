#include "r2pch.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/Editor.h"

#include "r2/Game/ECS/ECSCoordinator.h"

//Game Components
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"

//ImGui Components
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelHierarchyComponent.h"
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelDebugBoneComponent.h"

namespace r2::edit
{
	void RegisterAllEngineComponentWidgets(InspectorPanel& inspectorPanel)
	{
		Editor* editor = inspectorPanel.GetEditor();

		r2::ecs::ECSCoordinator* coordinator = editor->GetECSCoordinator();

		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::EditorComponent>(), InspectorPanelEditorComponent);
		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::TransformComponent>(), InspectorPanelTransformComponent);
		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::HierarchyComponent>(), InspectorPanelHierarchyComponent);
		inspectorPanel.RegisterComponentType(coordinator->GetComponentType<r2::ecs::DebugBoneComponent>(), InspectorPanelDebugBoneComponent);
	}
}