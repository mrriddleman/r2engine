#include "r2pch.h"
#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelSpotLightComponent.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/SelectionComponent.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Game/ECS/Components/LightUpdateComponent.h"
#include "r2/Core/Math/Transform.h"

#include "r2/Render/Renderer/Renderer.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace r2::edit
{

	InspectorPanelSpotLightDataSource::InspectorPanelSpotLightDataSource()
		:InspectorPanelComponentDataSource("Spot Light Component", 0, 0)
		, mnoptrEditor(nullptr)
		, mInspectorPanel(nullptr)
	{

	}

	InspectorPanelSpotLightDataSource::InspectorPanelSpotLightDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const InspectorPanel* inspectorPanel)
		:InspectorPanelComponentDataSource("Spot Light Component", coordinator->GetComponentType<ecs::SpotLightComponent>(), coordinator->GetComponentTypeHash<ecs::SpotLightComponent>())
		, mnoptrEditor(noptrEditor)
		, mInspectorPanel(inspectorPanel)
	{

	}

	void InspectorPanelSpotLightDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::SpotLightComponent* spotLightComponentPtr = static_cast<ecs::SpotLightComponent*>(componentData);

		R2_CHECK(spotLightComponentPtr != nullptr, "Should never happen");

		bool needsUpdate = false;

		if (ImGui::ColorEdit4("Light Color", glm::value_ptr(spotLightComponentPtr->lightProperties.color)))
		{
			needsUpdate = true;
		}

		ImGui::Text("Fall Off: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label falloff", &spotLightComponentPtr->lightProperties.fallOff, 0.01, 0.0f, 1.0f))
		{
			needsUpdate = true;
		}

		ImGui::Text("Intensity: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label intensity", &spotLightComponentPtr->lightProperties.intensity, 0.1, 0.0f, 1000.0f))
		{
			needsUpdate = true;
		}

		float innerCutoffDegrees = glm::degrees(glm::acos(spotLightComponentPtr->innerCutoffAngle));
		float outerCutoffDegrees = glm::degrees(glm::acos(spotLightComponentPtr->outerCutoffAngle));

		ImGui::Text("Inner Angle Cutoff");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label innercutoff", &innerCutoffDegrees, 1, 0.0, outerCutoffDegrees))
		{
			needsUpdate = true;
			spotLightComponentPtr->innerCutoffAngle = glm::cos(glm::radians(innerCutoffDegrees));
		}

		ImGui::Text("Outer Angle Cutoff");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label outercutoff", &outerCutoffDegrees, 1, innerCutoffDegrees, 89.0f))
		{
			needsUpdate = true;
			spotLightComponentPtr->outerCutoffAngle = glm::cos(glm::radians(outerCutoffDegrees));
		}

		bool castsShadows = spotLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.x > 0u;
		if (ImGui::Checkbox("Cast Shadows", &castsShadows))
		{
			spotLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.x = castsShadows ? 1u : 0u;
			needsUpdate = true;
		}

		bool softShadows = spotLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.y > 0u;
		if (ImGui::Checkbox("Soft Shadows", &softShadows))
		{
			spotLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.y = softShadows ? 1u : 0u;
			needsUpdate = true;
		}

		if (needsUpdate)
		{
			if (!coordinator->HasComponent<ecs::LightUpdateComponent>(theEntity))
			{
				ecs::LightUpdateComponent lightUpdate;
				lightUpdate.flags.Set(ecs::SPOT_LIGHT_UPDATE);
				coordinator->AddComponent<ecs::LightUpdateComponent>(theEntity, lightUpdate);
			}
		}

		const ecs::TransformComponent& transformComponent = coordinator->GetComponent<ecs::TransformComponent>(theEntity);

		glm::vec3 direction = glm::rotate(transformComponent.accumTransform.rotation, glm::vec3(0, 0, 1)); //0, 0, 1 - because that's the initial direction of a cone
		
		r2::draw::renderer::DrawCone(transformComponent.accumTransform.position - glm::vec3(0, 0, 0.5), direction, glm::acos(spotLightComponentPtr->outerCutoffAngle), 1, spotLightComponentPtr->lightProperties.color, true);
	}

	bool InspectorPanelSpotLightDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelSpotLightDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelSpotLightDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::SpotLightComponent>(theEntity);
	}

	void* InspectorPanelSpotLightDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelSpotLightDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::SelectionComponent* selectionComponent = coordinator->GetComponentPtr<ecs::SelectionComponent>(theEntity);

		if (selectionComponent != nullptr)
		{
			mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, 0, -1, theEntity, r2::sarr::At(*selectionComponent->selectedInstances, 0)));
		}

		coordinator->RemoveComponent<r2::ecs::SpotLightComponent>(theEntity);
	}

	void InspectorPanelSpotLightDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		//Empty
	}

	void InspectorPanelSpotLightDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::SpotLightComponent newSpotLightComponent;
		newSpotLightComponent.lightProperties.fallOff = 0;
		newSpotLightComponent.lightProperties.intensity = 1.0;

		r2::draw::SpotLight newSpotLight;
		newSpotLight.lightProperties = newSpotLightComponent.lightProperties;
		newSpotLight.position = glm::vec4(0, 0, 0, glm::cos(glm::radians(45.0f)));
		newSpotLight.direction = glm::vec4(0, 0, -1, glm::cos(glm::radians(50.0f)));

		newSpotLightComponent.innerCutoffAngle = newSpotLight.position.w;
		newSpotLightComponent.outerCutoffAngle = newSpotLight.direction.w;

		const ecs::TransformComponent* transformComponentPtr = coordinator->GetComponentPtr<ecs::TransformComponent>(theEntity);

		if (transformComponentPtr)
		{
			newSpotLight.position = glm::vec4(transformComponentPtr->accumTransform.position, newSpotLight.position.w);

			//now calculate the direction
			glm::vec3 newDirection = glm::rotate(transformComponentPtr->accumTransform.rotation, glm::vec3(newSpotLight.direction));
			newSpotLight.direction = glm::vec4(newDirection, newSpotLight.direction.w);
		}

		newSpotLightComponent.spotLightHandle = r2::draw::renderer::AddSpotLight(newSpotLight);

		newSpotLightComponent.lightProperties = r2::draw::renderer::GetSpotLightConstPtr(newSpotLightComponent.spotLightHandle)->lightProperties;

		//This should probably be an action?
		coordinator->AddComponent<ecs::SpotLightComponent>(theEntity, newSpotLightComponent);
	}

	void InspectorPanelSpotLightDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{
		//Empty
	}

}

#endif