#ifdef R2_EDITOR

#include "GridPositionComponentInspectorPanelDataSource.h"
#include "GridPositionComponent.h"
#include "../../r2engine/vendor/imgui/imgui.h"
#include "r2/Render/Renderer/Renderer.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/SelectionComponent.h"
#include "r2/Editor/EditorActions/SelectedEntityEditorAction.h"
#include "GameUtils.h"


InspectorPanelGridPositionDataSource::InspectorPanelGridPositionDataSource()
	:r2::edit::InspectorPanelComponentDataSource("Grid Position Component", 0, 0)
	, mnoptrEditor(nullptr)
	, mInspectorPanel(nullptr)
{

}

InspectorPanelGridPositionDataSource::InspectorPanelGridPositionDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const r2::edit::InspectorPanel* inspectorPanel)
	: r2::edit::InspectorPanelComponentDataSource("Grid Position Component", coordinator->GetComponentType<GridPositionComponent>(), coordinator->GetComponentTypeHash<GridPositionComponent>())
	, mnoptrEditor(noptrEditor)
	, mInspectorPanel(inspectorPanel)
{

}

void InspectorPanelGridPositionDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity, s32 instance)
{
	GridPositionComponent* gridPositionComponentPtr = static_cast<GridPositionComponent*>(componentData);
	GridPositionComponent& gridPositionComponent = *gridPositionComponentPtr;


	bool localChanged = false;
	bool globalChnaged = false;

	ImGui::Text("Local Grid Position: ");

	
	ImGui::Text("X: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label localx", &gridPositionComponent.localGridPosition.x))
	{
		localChanged = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("Y: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label localy", &gridPositionComponent.localGridPosition.y))
	{
		localChanged = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("Z: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label localz", &gridPositionComponent.localGridPosition.z))
	{
		localChanged = true;
	}
	ImGui::PopItemWidth();

	ImGui::Text("Global Grid Position: ");
	ImGui::Text("X: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label globalx", &gridPositionComponent.globalGridPosition.x))
	{
		globalChnaged = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("Y: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label globaly", &gridPositionComponent.globalGridPosition.y))
	{
		globalChnaged = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("Z: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label globalz", &gridPositionComponent.globalGridPosition.z))
	{
		globalChnaged = true;
	}
	ImGui::PopItemWidth();


	r2::ecs::TransformComponent* transformComponent = nullptr;
	if ( instance == -1)
	{
		transformComponent = coordinator->GetComponentPtr<r2::ecs::TransformComponent>(theEntity);
	}
	else
	{
		r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>* instancedTransformComponents = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(theEntity);
		if (instancedTransformComponents)
		{
			transformComponent = &r2::sarr::At(*instancedTransformComponents->instances, instance);
		}
	}

	if (transformComponent)
	{
		if (localChanged)
		{
			transformComponent->localTransform.position = utils::CalculateWorldPositionFromGridPosition(gridPositionComponent.localGridPosition);
			mnoptrEditor->GetSceneGraph().UpdateTransformForEntity(theEntity, r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY);
		}

		if (globalChnaged)
		{
			transformComponent->accumTransform.position = utils::CalculateWorldPositionFromGridPosition(gridPositionComponent.globalGridPosition);
			mnoptrEditor->GetSceneGraph().UpdateTransformForEntity(theEntity, r2::ecs::eTransformDirtyFlags::GLOBAL_TRANSFORM_DIRTY);
		}
	}
}


bool InspectorPanelGridPositionDataSource::InstancesEnabled() const
{
	return true;
}

u32 InspectorPanelGridPositionDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) const
{
	r2::ecs::InstanceComponentT<GridPositionComponent>* instancedGridPositionComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<GridPositionComponent>>(theEntity);

	if (instancedGridPositionComponent)
	{
		return instancedGridPositionComponent->numInstances;
	}

	return 0;
}

void* InspectorPanelGridPositionDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	return &coordinator->GetComponent<GridPositionComponent>(theEntity);
}

void* InspectorPanelGridPositionDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	r2::ecs::InstanceComponentT<GridPositionComponent>* instancedGridPositionComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<GridPositionComponent>>(theEntity);

	if (instancedGridPositionComponent)
	{
		R2_CHECK(i < instancedGridPositionComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedGridPositionComponent->numInstances);

		return &r2::sarr::At(*instancedGridPositionComponent->instances, i);
	}

	return nullptr;
}

void InspectorPanelGridPositionDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	r2::ecs::SelectionComponent* selectionComponent = coordinator->GetComponentPtr<r2::ecs::SelectionComponent>(theEntity);

	if (selectionComponent != nullptr)
	{

		mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, 0, -1, theEntity, r2::sarr::At(*selectionComponent->selectedInstances, 0)));

	}

	coordinator->RemoveComponent<GridPositionComponent>(theEntity);

	if (coordinator->HasComponent<r2::ecs::InstanceComponentT<GridPositionComponent>>(theEntity))
	{
		coordinator->RemoveComponent<r2::ecs::InstanceComponentT<GridPositionComponent>>(theEntity);
	}
}

void InspectorPanelGridPositionDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	r2::ecs::InstanceComponentT<GridPositionComponent>* instancedGridPositionComponent = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<GridPositionComponent>>(theEntity);
	if (!instancedGridPositionComponent)
	{
		return;
	}

	R2_CHECK(i < instancedGridPositionComponent->numInstances, "Trying to access instance: %u but only have %u instances", i, instancedGridPositionComponent->numInstances);

	r2::ecs::SelectionComponent* selectionComponent = coordinator->GetComponentPtr<r2::ecs::SelectionComponent>(theEntity);

	r2::sarr::RemoveElementAtIndexShiftLeft(*instancedGridPositionComponent->instances, i);
	instancedGridPositionComponent->numInstances--;

	if (selectionComponent != nullptr)
	{
		mnoptrEditor->PostNewAction(std::make_unique<r2::edit::SelectedEntityEditorAction>(mnoptrEditor, 0, -1, theEntity, static_cast<s32>(i)));
	}
}

void InspectorPanelGridPositionDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	GridPositionComponent newGridPositionComponent;

	r2::ecs::TransformComponent* transformComponent = coordinator->GetComponentPtr<r2::ecs::TransformComponent>(theEntity);

	if (transformComponent)
	{
		newGridPositionComponent.localGridPosition = utils::CalculateGridPosition(transformComponent->localTransform.position);
		newGridPositionComponent.globalGridPosition = utils::CalculateGridPosition(transformComponent->accumTransform.position);
	}
	else
	{
		newGridPositionComponent.localGridPosition = glm::ivec3(0);
		newGridPositionComponent.globalGridPosition = glm::ivec3(0);
	}
	//This should probably be an action?
	coordinator->AddComponent<GridPositionComponent>(theEntity, newGridPositionComponent);
}

void InspectorPanelGridPositionDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity, u32 numInstances)
{
	r2::ecs::InstanceComponentT<GridPositionComponent>* tempInstancedComponents = coordinator->GetComponentPtr<r2::ecs::InstanceComponentT<GridPositionComponent>>(theEntity);
	r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>* instancedTransformComponents = coordinator->GetComponentPtr < r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(theEntity);

	u32 instanceStart = 0;
	bool useTransformComponent = false;

	if (tempInstancedComponents && instancedTransformComponents)
	{
		if (instancedTransformComponents->numInstances >= (tempInstancedComponents->numInstances + numInstances))
		{
			instanceStart = tempInstancedComponents->numInstances;
			useTransformComponent = true;
		}
	}

	r2::ecs::InstanceComponentT<GridPositionComponent>* instancedTransformComponentToUse = r2::edit::AddNewInstanceCapacity<GridPositionComponent>(coordinator, theEntity, numInstances);

	for (u32 i = instanceStart; i < instanceStart + numInstances; ++i)
	{
		GridPositionComponent newGridPositionComponent;
		
		if (useTransformComponent)
		{
			const r2::ecs::TransformComponent& transformComponent = r2::sarr::At(*instancedTransformComponents->instances, i);
			newGridPositionComponent.localGridPosition = utils::CalculateGridPosition(transformComponent.localTransform.position);
			newGridPositionComponent.globalGridPosition = utils::CalculateGridPosition(transformComponent.accumTransform.position);
		}
		else
		{
			newGridPositionComponent.localGridPosition = glm::ivec3(0);
			newGridPositionComponent.globalGridPosition = glm::ivec3(0);
		}

		r2::sarr::Push(*instancedTransformComponentToUse->instances, newGridPositionComponent);
	}

	instancedTransformComponentToUse->numInstances += numInstances;
}

#endif