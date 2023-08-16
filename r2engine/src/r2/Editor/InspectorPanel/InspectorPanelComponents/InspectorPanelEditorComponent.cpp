#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelEditorComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/EditorComponent.h"

#include "imgui.h"

namespace r2::edit
{
	void InspectorPanelEditorComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::EditorComponent& editorComponent = coordinator->GetComponent<r2::ecs::EditorComponent>(theEntity);

		char buf[1024];
		
		strncpy(buf, editorComponent.editorName.c_str(), 1024);

		if (ImGui::CollapsingHeader("Editor Component"))
		{
			ImGui::InputText("Editor Name", buf, 1024);

			editorComponent.editorName = buf;

			bool showEntity = editorComponent.flags.IsSet(r2::ecs::EDITOR_FLAG_SHOW_ENTITY);

			ImGui::Checkbox("Show Entity", &showEntity);

			bool entityEnabled = editorComponent.flags.IsSet(r2::ecs::EDITOR_FLAG_ENABLE_ENTITY);

			ImGui::Checkbox("Entity Enabled", &entityEnabled);

			bool selectedEntity = editorComponent.flags.IsSet(r2::ecs::EDITOR_FLAG_SELECTED_ENTITY);

			ImGui::Checkbox("Entity Selected", &selectedEntity);

			editorComponent.flags.Clear();

			if (showEntity)
			{
				editorComponent.flags.Set(r2::ecs::EDITOR_FLAG_SHOW_ENTITY);
			}

			if (entityEnabled)
			{
				editorComponent.flags.Set(r2::ecs::EDITOR_FLAG_ENABLE_ENTITY);
			}

			if (selectedEntity)
			{
				editorComponent.flags.Set(r2::ecs::EDITOR_FLAG_SELECTED_ENTITY);
			}
		}
	}
}
#endif