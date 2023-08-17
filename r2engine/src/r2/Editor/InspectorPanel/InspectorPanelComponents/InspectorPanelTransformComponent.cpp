#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "imgui.h"
#include <glm/gtc/quaternion.hpp>


namespace r2::edit
{
	void TransformImGuiWidget(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity entity, int id, r2::ecs::TransformComponent& transformComponent)
	{
		const r2::ecs::EditorComponent* editorComponent = coordinator->GetComponentPtr<r2::ecs::EditorComponent>(entity);

		std::string nodeName = std::string("Entity - ") + std::to_string(entity) + std::string(" - Transform Instance - ") + std::to_string(id);
		if (editorComponent)
		{
			nodeName = editorComponent->editorName + std::string(" - Transform Instance - ") + std::to_string(id);
		}
		if (ImGui::TreeNodeEx(nodeName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth, "%s", nodeName.c_str()))
		{
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
			ImGui::DragFloat("##label xrot", &eulerAngles.x, 0.1f, -179.9f, 179.9f);

			ImGui::Text("Y Rot: ");
			ImGui::SameLine();
			ImGui::DragFloat("##label yrot", &eulerAngles.y, 0.1f, -179.9f, 179.9f);

			ImGui::Text("Z Rot: ");
			ImGui::SameLine();
			ImGui::DragFloat("##label zrot", &eulerAngles.z, 0.1f, -179.9f, 179.9f);

			transformComponent.localTransform.rotation = glm::quat(glm::radians(eulerAngles));

			ImGui::TreePop();
		}
	}

	void InspectorPanelTransformComponent(Editor* editor, r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::TransformComponent& transformComponent = coordinator->GetComponent<r2::ecs::TransformComponent>(theEntity);

		if (ImGui::CollapsingHeader("Transform Component"))
		{
			TransformImGuiWidget(coordinator, theEntity, 0, transformComponent);

			r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>* instancedTransforms = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT< r2::ecs::TransformComponent>>(theEntity);

			if (instancedTransforms)
			{
				const auto numInstances = instancedTransforms->numInstances;

				for (u32 i = 0; i < numInstances; ++i)
				{
					r2::ecs::TransformComponent& transformComponent = r2::sarr::At(*instancedTransforms->instances, i);
					TransformImGuiWidget(coordinator, theEntity, i + 1, transformComponent);
				}
			}

			r2::ecs::TransformDirtyComponent dirty;
			coordinator->AddComponent<r2::ecs::TransformDirtyComponent>(theEntity, dirty);
		}
	}
}

#endif