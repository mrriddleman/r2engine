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
#include "r2/Editor/EditorEvents/EditorEntityEvents.h"
#include "../../r2engine/vendor/ImGuizmo/ImGuizmo.h"
#include "r2/Game/SceneGraph/SceneGraph.h"


void UpdateGridPositionHierarchy(GridPositionComponent& gridPositionComponent, r2::ecs::ECSCoordinator* coordinator, r2::ecs::SceneGraph& sceneGraph, r2::ecs::Entity theEntity, s32 instance, bool isLocal)
{
	const r2::ecs::HierarchyComponent* entityHeirarchComponent = coordinator->GetComponentPtr<r2::ecs::HierarchyComponent>(theEntity);
	//get the parent entity

	r2::ecs::Entity parentEntity = r2::ecs::INVALID_ENTITY;
	if (entityHeirarchComponent)
	{
		parentEntity = entityHeirarchComponent->parent;
	}

	GridPositionComponent* parentGridComponent = coordinator->GetComponentPtr<GridPositionComponent>(parentEntity);

	if (isLocal)
	{
		if (parentGridComponent)
		{
			gridPositionComponent.globalGridPosition = parentGridComponent->globalGridPosition + gridPositionComponent.localGridPosition;
		}
		else
		{
			gridPositionComponent.globalGridPosition = gridPositionComponent.localGridPosition;
		}
	}
	else
	{
		if (parentGridComponent)
		{
			gridPositionComponent.localGridPosition = gridPositionComponent.globalGridPosition - parentGridComponent->globalGridPosition;
		}
		else
		{
			gridPositionComponent.localGridPosition = gridPositionComponent.globalGridPosition;
		}
	}

	if (instance == -1)
	{
		//update all of the children

		r2::SArray<r2::ecs::Entity>* children = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::ecs::Entity, r2::ecs::MAX_NUM_ENTITIES);

		sceneGraph.GetAllChildrenForEntity(theEntity, *children);

		const auto  numChildren = r2::sarr::Size(*children);

		for (u32 i = 0; i < numChildren; ++i)
		{
			r2::ecs::Entity child = r2::sarr::At(*children, i);

			GridPositionComponent& childGridPosition = coordinator->GetComponent<GridPositionComponent>(child);

			UpdateGridPositionHierarchy(childGridPosition, coordinator, sceneGraph, child, -1, true);
		}

		FREE(children, *MEM_ENG_SCRATCH_PTR);
	}
}

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


	glm::ivec3* gridPosition = &gridPositionComponent.localGridPosition;
	r2::ecs::eTransformDirtyFlags dirtyFlags = r2::ecs::LOCAL_TRANSFORM_DIRTY;
	if (mInspectorPanel->GetCurrentMode() == ImGuizmo::MODE::WORLD)
	{
		gridPosition = &gridPositionComponent.globalGridPosition;
		dirtyFlags = r2::ecs::GLOBAL_TRANSFORM_DIRTY;
	}

	bool changed = false;

	ImGui::Text("Grid Position: ");

	
	ImGui::Text("X: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label localx", &gridPosition->x))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("Y: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label localy", &gridPosition->y))
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("Z: ");
	ImGui::SameLine();
	ImGui::PushItemWidth(80);
	if (ImGui::DragInt("##label localz", &gridPosition->z))
	{
		changed = true;
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
		if (changed)
		{
			if (dirtyFlags == r2::ecs::LOCAL_TRANSFORM_DIRTY)
			{
				transformComponent->localTransform.position = utils::CalculateWorldPositionFromGridPosition(gridPositionComponent.localGridPosition);
			}
			else
			{
				transformComponent->accumTransform.position = utils::CalculateWorldPositionFromGridPosition(gridPositionComponent.globalGridPosition);
			}
			
			mnoptrEditor->GetSceneGraph().UpdateTransformForEntity(theEntity, dirtyFlags);
		}
	}

	if (changed)
	{
		//@TODO(Serge): calculate the full grid position based on the hierarchy of the entity
		UpdateGridPositionHierarchy(gridPositionComponent, coordinator, mnoptrEditor->GetSceneGraph(), theEntity, instance, dirtyFlags == r2::ecs::LOCAL_TRANSFORM_DIRTY);
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
	r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>* instancedTransformComponents = coordinator->GetComponentPtr < r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(theEntity);

	r2::ecs::InstanceComponentT<GridPositionComponent>* instancedTransformComponentToUse = r2::edit::AddNewInstanceCapacity<GridPositionComponent>(coordinator, theEntity, numInstances);
	
	u32 instanceStart = 0;
	bool useTransformComponent = false;

	if (instancedTransformComponentToUse && instancedTransformComponents)
	{
		if (instancedTransformComponents->numInstances >= (instancedTransformComponentToUse->numInstances + numInstances))
		{
			instanceStart = instancedTransformComponentToUse->numInstances;
			useTransformComponent = true;
		}
	}

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

	if (instancedTransformComponentToUse)
	{
		instancedTransformComponentToUse->numInstances += numInstances;
	}
}

bool InspectorPanelGridPositionDataSource::OnEvent(r2::evt::Event& e)
{
	r2::evt::EventDispatcher dispatch(e);

	dispatch.Dispatch<r2::evt::EditorEntityTransformComponentChangedEvent>([this](const r2::evt::EditorEntityTransformComponentChangedEvent& entityTransformedEvent)
		{
			r2::ecs::Entity entity = entityTransformedEvent.GetEntity();
			r2::ecs::eTransformDirtyFlags dirtyFlags = entityTransformedEvent.GetTransformDirtyFlags();
			GridPositionComponent* gridPositionComponent = nullptr;

			const r2::ecs::TransformComponent* transformComponent = nullptr;

			if (entityTransformedEvent.GetInstance() < 0)
			{
				transformComponent = &mnoptrEditor->GetECSCoordinator()->GetComponent<r2::ecs::TransformComponent>(entity);
				gridPositionComponent = mnoptrEditor->GetECSCoordinator()->GetComponentPtr<GridPositionComponent>(entity);
			}
			else
			{
				const r2::ecs::InstanceComponentT<r2::ecs::TransformComponent> instancedTransformComponents = mnoptrEditor->GetECSCoordinator()->GetComponent<r2::ecs::InstanceComponentT<r2::ecs::TransformComponent>>(entity);
				transformComponent = &r2::sarr::At(*instancedTransformComponents.instances, entityTransformedEvent.GetInstance());

				const r2::ecs::InstanceComponentT<GridPositionComponent>* instancedGridPositionComponents = mnoptrEditor->GetECSCoordinator()->GetComponentPtr<r2::ecs::InstanceComponentT<GridPositionComponent>>(entity);
				if (instancedGridPositionComponents)
				{
					gridPositionComponent = &r2::sarr::At(*instancedGridPositionComponents->instances, entityTransformedEvent.GetInstance());
				}
			}
				

			if (!gridPositionComponent)
			{
				return false;
			}

			if ((dirtyFlags & r2::ecs::eTransformDirtyFlags::GLOBAL_TRANSFORM_DIRTY) == r2::ecs::eTransformDirtyFlags::GLOBAL_TRANSFORM_DIRTY)
			{

				gridPositionComponent->globalGridPosition = utils::CalculateGridPosition(transformComponent->accumTransform.position);

				UpdateGridPositionHierarchy(*gridPositionComponent, mnoptrEditor->GetECSCoordinator(), mnoptrEditor->GetSceneGraph(), entity, entityTransformedEvent.GetInstance(), false);
				//@TODO(Serge): calculate the full grid position based on the hierarchy of the entity
			}
			else if ((dirtyFlags & r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY) == r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY)
			{
				gridPositionComponent->localGridPosition = utils::CalculateGridPosition(transformComponent->localTransform.position);

				//@TODO(Serge): calculate the full grid position based on the hierarchy of the entity
				UpdateGridPositionHierarchy(*gridPositionComponent, mnoptrEditor->GetECSCoordinator(), mnoptrEditor->GetSceneGraph(), entity, entityTransformedEvent.GetInstance(), true);
			}
			else
			{
				R2_CHECK(false, "Not supported");
			}

			return false;
		});

	dispatch.Dispatch<r2::evt::EditorEntityAttachedToNewParentEvent>([this](const r2::evt::EditorEntityAttachedToNewParentEvent& entityAttachedToNewParentEvent)
		{
			r2::ecs::Entity entity = entityAttachedToNewParentEvent.GetEntity();
		
			GridPositionComponent* gridPositionComponent = mnoptrEditor->GetECSCoordinator()->GetComponentPtr<GridPositionComponent>(entity);

			if (!gridPositionComponent)
			{
				return false;
			}

			r2::ecs::Entity newParent = entityAttachedToNewParentEvent.GetNewParent();

			if (newParent != r2::ecs::INVALID_ENTITY)
			{
				GridPositionComponent* parentGridPositionComponent = mnoptrEditor->GetECSCoordinator()->GetComponentPtr<GridPositionComponent>(newParent);
				gridPositionComponent->localGridPosition = gridPositionComponent->globalGridPosition - parentGridPositionComponent->globalGridPosition;
			}
			else
			{
				gridPositionComponent->localGridPosition = gridPositionComponent->globalGridPosition;
			}
			
			return false;
		});


	return false;
}

#endif