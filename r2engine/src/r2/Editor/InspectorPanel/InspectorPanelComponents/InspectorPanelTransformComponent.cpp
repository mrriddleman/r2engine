#include "r2pch.h"

#ifdef R2_EDITOR
#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelTransformComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"

#include "imgui.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace r2::edit
{
	/*
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = glm::quat(1, 0, 0, 0);
	*/




	void InspectorPanelTransformComponent(r2::ecs::Entity theEntity, r2::ecs::ECSCoordinator* coordinator)
	{
		r2::ecs::TransformComponent& transformComponent = coordinator->GetComponent<r2::ecs::TransformComponent>(theEntity);

		if (ImGui::CollapsingHeader("Transform Component"))
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

			r2::ecs::TransformDirtyComponent dirty;
			coordinator->AddComponent<r2::ecs::TransformDirtyComponent>(theEntity, dirty);
		}
	}
}

#endif