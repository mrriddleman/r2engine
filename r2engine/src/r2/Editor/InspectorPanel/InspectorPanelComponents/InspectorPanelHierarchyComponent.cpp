#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelHierarchyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "imgui.h"

#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/AttachEntityEditorAction.h"

namespace r2::edit
{
	void InspectorPanelHierarchyComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		const r2::SArray<r2::ecs::Entity>& entities = coordinator->GetAllLivingEntities();

		const r2::ecs::HierarchyComponent& hierarchyComponent = coordinator->GetComponent<r2::ecs::HierarchyComponent>(theEntity);

		std::string parentString = "No Parent";

		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(hierarchyComponent.parent);

		const u32 numEntities = r2::sarr::Size(entities);

		if (editorComponent)
		{
			parentString = editorComponent->editorName;
		}

		if (ImGui::CollapsingHeader("Hierarchy Component"))
		{
			ImGui::Text("Parent: ");
			ImGui::SameLine();

			if (ImGui::BeginCombo("##label parent", parentString.c_str()))
			{
				if (ImGui::Selectable("No Parent", hierarchyComponent.parent == r2::ecs::INVALID_ENTITY))
				{
					//set no parent
					editor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(editor, theEntity, hierarchyComponent.parent, r2::ecs::INVALID_ENTITY));
				}

				for (u32 i = 0; i < numEntities; ++i)
				{
					r2::ecs::Entity nextEntity = r2::sarr::At(entities, i);

					if (nextEntity == theEntity)
					{
						continue;
					}

					std::string entityName = std::string("Entity - ") + std::to_string(nextEntity);

					const r2::ecs::EditorComponent* nextEditorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(nextEntity);
					if (nextEditorComponent)
					{
						entityName = nextEditorComponent->editorName;
					}

					if (ImGui::Selectable(entityName.c_str(), hierarchyComponent.parent == nextEntity))
					{
						//set the parent
						if (hierarchyComponent.parent != nextEntity)
						{
							editor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(editor, theEntity, hierarchyComponent.parent, nextEntity));
						}
					}
				}

				ImGui::EndCombo();
			}
		}
	}
}


#endif