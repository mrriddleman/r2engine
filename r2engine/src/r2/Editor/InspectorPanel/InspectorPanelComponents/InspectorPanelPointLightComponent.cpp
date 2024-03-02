#include "r2pch.h"

#ifdef R2_EDITOR

#include "r2/Editor/InspectorPanel/InspectorPanelComponents/InspectorPanelPointLightComponent.h"
#include "r2/Editor/Editor.h"
#include "r2/Game/ECS/Components/EditorComponent.h"
#include "r2/Game/ECS/Components/SelectionComponent.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "r2/Game/ECS/Components/LightUpdateComponent.h"

#include "r2/Render/Renderer/Renderer.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

namespace r2::edit
{

	InspectorPanelPointLightDataSource::InspectorPanelPointLightDataSource()
		:InspectorPanelComponentDataSource("Point Light Component", 0, 0)
		, mnoptrEditor(nullptr)
		, mInspectorPanel(nullptr)
	{

	}

	InspectorPanelPointLightDataSource::InspectorPanelPointLightDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const InspectorPanel* inspectorPanel)
		:InspectorPanelComponentDataSource("Point Light Component", coordinator->GetComponentType<ecs::PointLightComponent>(), coordinator->GetComponentTypeHash<ecs::PointLightComponent>())
		, mnoptrEditor(noptrEditor)
		, mInspectorPanel(inspectorPanel)
	{

	}

	void InspectorPanelPointLightDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, s32 instance)
	{
		ecs::PointLightComponent* pointLightComponentPtr = static_cast<ecs::PointLightComponent*>(componentData);

		R2_CHECK(pointLightComponentPtr != nullptr, "Should never happen");
		
		bool needsUpdate = false;

		if (ImGui::ColorEdit4("Light Color", glm::value_ptr(pointLightComponentPtr->lightProperties.color)))
		{
			needsUpdate = true;
		}

		ImGui::Text("Fall Off: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label falloff", &pointLightComponentPtr->lightProperties.fallOff, 0.01, 0.0f, 1.0f))
		{
			needsUpdate = true;
		}
		
		ImGui::Text("Intensity: ");
		ImGui::SameLine();
		if (ImGui::DragFloat("##label intensity", &pointLightComponentPtr->lightProperties.intensity, 0.1, 0.0f, 1000.0f))
		{
			needsUpdate = true;
		}

		bool castsShadows = pointLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.x > 0u;
		if (ImGui::Checkbox("Cast Shadows", &castsShadows))
		{
			pointLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.x = castsShadows ? 1u : 0u;
			needsUpdate = true;
		}

		bool softShadows = pointLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.y > 0u;
		if (ImGui::Checkbox("Soft Shadows", &softShadows))
		{
			pointLightComponentPtr->lightProperties.castsShadowsUseSoftShadows.y = softShadows ? 1u : 0u;
			needsUpdate = true;
		}

		if (needsUpdate)
		{
			if (!coordinator->HasComponent<ecs::LightUpdateComponent>(theEntity))
			{
				ecs::LightUpdateComponent lightUpdate;
				lightUpdate.flags.Set(ecs::POINT_LIGHT_UPDATE);
				coordinator->AddComponent<ecs::LightUpdateComponent>(theEntity, lightUpdate);
			}
		}

		const ecs::TransformComponent& transformComponent = coordinator->GetComponent<ecs::TransformComponent>(theEntity);

		r2::draw::renderer::DrawSphere(transformComponent.accumTransform.position, 1, pointLightComponentPtr->lightProperties.color, true);
	}

	bool InspectorPanelPointLightDataSource::InstancesEnabled() const
	{
		return false;
	}

	u32 InspectorPanelPointLightDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity) const
	{
		return 0;
	}

	void* InspectorPanelPointLightDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return &coordinator->GetComponent<ecs::PointLightComponent>(theEntity);
	}

	void* InspectorPanelPointLightDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		return nullptr;
	}

	void InspectorPanelPointLightDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::SelectionComponent* selectionComponent = coordinator->GetComponentPtr<ecs::SelectionComponent>(theEntity);

		if (selectionComponent != nullptr)
		{
			mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, 0, -1, theEntity, r2::sarr::At(*selectionComponent->selectedInstances, 0)));
		}

		coordinator->RemoveComponent<r2::ecs::PointLightComponent>(theEntity);
	}

	void InspectorPanelPointLightDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		//Empty
	}

	void InspectorPanelPointLightDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity)
	{
		ecs::PointLightComponent newPointLightComponent;
		newPointLightComponent.lightProperties.intensity = 1.0f;
		newPointLightComponent.lightProperties.fallOff = 0;

		r2::draw::PointLight newPointLight;
		newPointLight.lightProperties = newPointLightComponent.lightProperties;
		newPointLight.position = glm::vec4(0);


		const ecs::TransformComponent* transformComponentPtr = coordinator->GetComponentPtr<ecs::TransformComponent>(theEntity);
		if (transformComponentPtr)
		{
			newPointLight.position = glm::vec4(transformComponentPtr->accumTransform.position, 1);
		}

		newPointLightComponent.pointLightHandle = r2::draw::renderer::AddPointLight(newPointLight);

		newPointLightComponent.lightProperties = r2::draw::renderer::GetPointLightConstPtr(newPointLightComponent.pointLightHandle)->lightProperties;

		//This should probably be an action?
		coordinator->AddComponent<ecs::PointLightComponent>(theEntity, newPointLightComponent);




		
	}

	void InspectorPanelPointLightDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, ecs::Entity theEntity, u32 numInstances)
	{
		//Empty
	}

}


#endif