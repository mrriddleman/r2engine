#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/SelectionComponent.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Editor/Editor.h"
#include "imgui.h"
#include <glm/gtc/quaternion.hpp>

namespace r2::edit
{
	InspectorPanelTransformDataSource::InspectorPanelTransformDataSource()
		:InspectorPanelComponentDataSource("Transform Component", 0, 0)
	{
	}

	InspectorPanelTransformDataSource::InspectorPanelTransformDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator)
		:InspectorPanelComponentDataSource("Transform Component", coordinator->GetComponentType<ecs::TransformComponent>(), coordinator->GetComponentTypeHash<ecs::TransformComponent>())
		,mnoptrEditor(noptrEditor)
	{
	}

	void InspectorPanelTransformDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::TransformComponent* transformComponentPtr = static_cast<ecs::TransformComponent*>(componentData);
		ecs::TransformComponent& transformComponent = *transformComponentPtr;

		ImGui::Text("Position");

		ImGui::Text("X Pos: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label xpos", &transformComponent.localTransform.position.x, 0.1f);

		ImGui::Text("Y Pos: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label ypos", &transformComponent.localTransform.position.y, 0.1f);

		ImGui::Text("Z Pos: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label zpos", &transformComponent.localTransform.position.z, 0.1f);

		ImGui::Text("Scale");

		ImGui::Text("X Scale: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label xscale", &transformComponent.localTransform.scale.x, 0.01f, 0.01f, 1.0f);

		ImGui::Text("Y Scale: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label yscale", &transformComponent.localTransform.scale.y, 0.01f, 0.01f, 1.0f);

		ImGui::Text("Z Scale: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label zscale", &transformComponent.localTransform.scale.z, 0.01f, 0.01f, 1.0f);

		ImGui::Text("Rotation");

		glm::vec3 eulerAngles = glm::eulerAngles(transformComponent.localTransform.rotation);
		eulerAngles = glm::degrees(eulerAngles);

		ImGui::Text("X Rot: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label xrot", &eulerAngles.x, 0.1f, -179.999f, 179.999f);

		ImGui::Text("Y Rot: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label yrot", &eulerAngles.y, 0.1f, -179.999f, 179.999f);

		ImGui::Text("Z Rot: ");
		ImGui::SameLine();
		ImGui::DragFloat("##label zrot", &eulerAngles.z, 0.1f, -179.999f, 179.999f);

		transformComponent.localTransform.rotation = glm::quat(glm::radians(eulerAngles));

		if (!coordinator->HasComponent<ecs::TransformDirtyComponent>(theEntity))
		{
			ecs::TransformDirtyComponent transformDirtyComponent;
			coordinator->AddComponent<ecs::TransformDirtyComponent>(theEntity, transformDirtyComponent);
		}
	}

	bool InspectorPanelTransformDataSource::InstancesEnabled() const
	{
		return true;
	}

	u32 InspectorPanelTransformDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		ecs::InstanceComponentT<ecs::TransformComponent>* instancedTransformComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity);

		if (instancedTransformComponent)
		{
			return instancedTransformComponent->numInstances;
		}

		return 0;
	}

	void* InspectorPanelTransformDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::TransformComponent>(theEntity);
	}

	void* InspectorPanelTransformDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::InstanceComponentT<ecs::TransformComponent>* instancedTransformComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity);

		if (instancedTransformComponent)
		{
			R2_CHECK(i < instancedTransformComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedTransformComponent->numInstances);

			return &r2::sarr::At(*instancedTransformComponent->instances, i);
		}

		return nullptr;
	}

	void InspectorPanelTransformDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::SelectionComponent* selectionComponent = coordinator->GetComponentPtr<ecs::SelectionComponent>(theEntity);

		if (selectionComponent != nullptr)
		{

			mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, 0, -1, theEntity, r2::sarr::At(*selectionComponent->selectedInstances, 0)));
			
		}

		coordinator->RemoveComponent<r2::ecs::TransformComponent>(theEntity);

		if (coordinator->HasComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity))
		{
			coordinator->RemoveComponent<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity);
		}
	}

	void InspectorPanelTransformDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::InstanceComponentT<ecs::TransformComponent>* instancedTransformComponent = coordinator->GetComponentPtr<ecs::InstanceComponentT<ecs::TransformComponent>>(theEntity);
		if (!instancedTransformComponent)
		{
			return;
		}

		R2_CHECK(i < instancedTransformComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedTransformComponent->numInstances);


		ecs::SelectionComponent* selectionComponent = coordinator->GetComponentPtr<ecs::SelectionComponent>(theEntity);

		r2::sarr::RemoveElementAtIndexShiftLeft(*instancedTransformComponent->instances, i);
		instancedTransformComponent->numInstances--;

		if (selectionComponent != nullptr)
		{
			mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, 0, -1, theEntity, static_cast<s32>(i)));
		}
	}

	void InspectorPanelTransformDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::TransformComponent newTransformComponent;

		//This should probably be an action?
		coordinator->AddComponent<ecs::TransformComponent>(theEntity, newTransformComponent);
	}

	void InspectorPanelTransformDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{
		r2::ecs::InstanceComponentT<ecs::TransformComponent>* instancedTransformComponentToUse = AddNewInstanceCapacity<ecs::TransformComponent>(coordinator, theEntity, numInstances);

		for (u32 i = 0; i < numInstances; ++i)
		{
			ecs::TransformComponent newTransformComponent;

			r2::sarr::Push(*instancedTransformComponentToUse->instances, newTransformComponent);
		}

		instancedTransformComponentToUse->numInstances += numInstances;
	}
}

#endif