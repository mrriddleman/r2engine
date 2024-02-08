#include "r2pch.h"
#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelHierarchyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"

#include "imgui.h"

#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorActions/AttachEntityEditorAction.h"
#include "r2/Editor/EditorActions/DetachEntityEditorAction.h"

namespace r2::edit
{
//	void InspectorPanelHierarchyComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
//	{
//		const r2::SArray<r2::ecs::Entity>& entities = coordinator->GetAllLivingEntities();
//
//		const r2::ecs::HierarchyComponent& hierarchyComponent = coordinator->GetComponent<r2::ecs::HierarchyComponent>(theEntity);
//
//		std::string parentString = "No Parent";
//
//		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(hierarchyComponent.parent);
//
//		const u32 numEntities = r2::sarr::Size(entities);
//
//		if (editorComponent)
//		{
//			parentString = editorComponent->editorName;
//		}
//
////		if (ImGui::CollapsingHeader("Hierarchy Component"))
//		{
//			ImGui::Text("Parent: ");
//			ImGui::SameLine();
//
//			if (ImGui::BeginCombo("##label parent", parentString.c_str()))
//			{
//				if (ImGui::Selectable("No Parent", hierarchyComponent.parent == r2::ecs::INVALID_ENTITY))
//				{
//					//set no parent
//					editor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(editor, theEntity, hierarchyComponent.parent, r2::ecs::INVALID_ENTITY));
//				}
//
//				for (u32 i = 0; i < numEntities; ++i)
//				{
//					r2::ecs::Entity nextEntity = r2::sarr::At(entities, i);
//
//					if (nextEntity == theEntity)
//					{
//						continue;
//					}
//
//					std::string entityName = std::string("Entity - ") + std::to_string(nextEntity);
//
//					const r2::ecs::EditorComponent* nextEditorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(nextEntity);
//					if (nextEditorComponent)
//					{
//						entityName = nextEditorComponent->editorName;
//					}
//
//					if (ImGui::Selectable(entityName.c_str(), hierarchyComponent.parent == nextEntity))
//					{
//						//set the parent
//						if (hierarchyComponent.parent != nextEntity)
//						{
//							editor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(editor, theEntity, hierarchyComponent.parent, nextEntity));
//						}
//					}
//				}
//
//				ImGui::EndCombo();
//			}
//		}
//	}

	InspectorPanelHierarchyComponentDataSource::InspectorPanelHierarchyComponentDataSource()
		:InspectorPanelComponentDataSource("Hierarchy Component", 0, 0)
		,mnoptrEditor(nullptr)
	{

	}

	InspectorPanelHierarchyComponentDataSource::InspectorPanelHierarchyComponentDataSource(r2::Editor* noptrEditor, ecs::ECSCoordinator* coordinator)
		:InspectorPanelComponentDataSource("Hierarchy Component", coordinator->GetComponentType<ecs::HierarchyComponent>(), coordinator->GetComponentTypeHash<ecs::HierarchyComponent>())
		,mnoptrEditor(noptrEditor)
	{

	}

	void InspectorPanelHierarchyComponentDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
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

		ImGui::Text("Parent: ");
		ImGui::SameLine();

		if (ImGui::BeginCombo("##label parent", parentString.c_str()))
		{
			if (ImGui::Selectable("No Parent", hierarchyComponent.parent == r2::ecs::INVALID_ENTITY))
			{
				//set no parent
				mnoptrEditor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(mnoptrEditor, theEntity, hierarchyComponent.parent, r2::ecs::INVALID_ENTITY, ecs::eHierarchyAttachmentType::KEEP_GLOBAL));
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
						mnoptrEditor->PostNewAction(std::make_unique<edit::AttachEntityEditorAction>(mnoptrEditor, theEntity, hierarchyComponent.parent, nextEntity, ecs::eHierarchyAttachmentType::KEEP_GLOBAL));
					}
				}
			}

			ImGui::EndCombo();
		}
		
	}

	bool InspectorPanelHierarchyComponentDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelHierarchyComponentDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelHierarchyComponentDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::HierarchyComponent>(theEntity);
	}

	void* InspectorPanelHierarchyComponentDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelHierarchyComponentDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		const ecs::HierarchyComponent& hierarchyComponent = coordinator->GetComponent<ecs::HierarchyComponent>(theEntity);

		mnoptrEditor->PostNewAction(std::make_unique<edit::DetachEntityEditorAction>(mnoptrEditor, theEntity, hierarchyComponent.parent, ecs::eHierarchyAttachmentType::KEEP_GLOBAL));

		//This should probably be an action?
		coordinator->RemoveComponent<ecs::HierarchyComponent>(theEntity);
	}

	void InspectorPanelHierarchyComponentDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{

	}

	void InspectorPanelHierarchyComponentDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::HierarchyComponent newHierarchyComponent;
		newHierarchyComponent.parent = ecs::INVALID_ENTITY;

		//This should probably be an action?
		coordinator->AddComponent<ecs::HierarchyComponent>(theEntity, newHierarchyComponent);
	}

	void InspectorPanelHierarchyComponentDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{

	}
}


#endif