#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/SelectionComponent.h"
#include "r2/Editor/EditorInspectorPanel.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Editor/Editor.h"
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include <glm/gtc/quaternion.hpp>

namespace r2::edit
{
	InspectorPanelTransformDataSource::InspectorPanelTransformDataSource()
		:InspectorPanelComponentDataSource("Transform Component", 0, 0)
		,mnoptrEditor(nullptr)
		, mInspectorPanel(nullptr)
	{
	}

	InspectorPanelTransformDataSource::InspectorPanelTransformDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const InspectorPanel* inspectorPanel)
		:InspectorPanelComponentDataSource("Transform Component", coordinator->GetComponentType<ecs::TransformComponent>(), coordinator->GetComponentTypeHash<ecs::TransformComponent>())
		,mnoptrEditor(noptrEditor)
		,mInspectorPanel(inspectorPanel)
	{
	}

	void InspectorPanelTransformDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance)
	{
		ecs::TransformComponent* transformComponentPtr = static_cast<ecs::TransformComponent*>(componentData);

		r2::math::Transform* transform = &transformComponentPtr->localTransform;

		ecs::eTransformDirtyFlags dirtyFlags = ecs::LOCAL_TRANSFORM_DIRTY;
		if (mInspectorPanel->GetCurrentMode() == ImGuizmo::MODE::WORLD)
		{
			transform = &transformComponentPtr->accumTransform;
			dirtyFlags = ecs::GLOBAL_TRANSFORM_DIRTY;
		}
		
		bool needsUpdate = false;

		ImGui::Text("Position");

		ImGui::Text("X Pos: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label xpos", &transform->position.x, 0.1f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Y Pos: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label ypos", &transform->position.y, 0.1f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Z Pos: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label zpos", &transform->position.z, 0.1f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Scale");

		ImGui::Text("X Scale: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label xscale", &transform->scale.x, 0.01f, 0.01f, 1.0f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Y Scale: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label yscale", &transform->scale.y, 0.01f, 0.01f, 1.0f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Z Scale: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label zscale", &transform->scale.z, 0.01f, 0.01f, 1.0f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Rotation");

		glm::vec3 eulerAngles = glm::eulerAngles(transform->rotation);
		eulerAngles = glm::degrees(eulerAngles);

		ImGui::Text("X Rot: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label xrot", &eulerAngles.x, 0.1f, -179.999f, 179.999f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Y Rot: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label yrot", &eulerAngles.y, 0.1f, -179.999f, 179.999f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Z Rot: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label zrot", &eulerAngles.z, 0.1f, -179.999f, 179.999f))
		{
			needsUpdate = true;
		}

		transform->rotation = glm::quat(glm::radians(eulerAngles));

		if (needsUpdate && !coordinator->HasComponent<ecs::TransformDirtyComponent>(theEntity))
		{

			ecs::TransformDirtyComponent transformDirtyComponent;
			transformDirtyComponent.dirtyFlags = dirtyFlags;

			mnoptrEditor->GetSceneGraph().UpdateTransformForEntity(theEntity, dirtyFlags);


			evt::EditorEntityTransformComponentChangedEvent e(theEntity, instance, dirtyFlags);

			mnoptrEditor->PostEditorEvent(e);
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