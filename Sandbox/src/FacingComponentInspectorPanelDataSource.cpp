#ifdef R2_EDITOR
#include "FacingComponentInspectorPanelDataSource.h"
#include "FacingComponent.h"
#include "../../r2engine/vendor/imgui/imgui.h"
#include "r2/Render/Renderer/Renderer.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include <glm/gtx/quaternion.hpp>
#include "r2/Core/Math/MathUtils.h"

InspectorPanelFacingDataSource::InspectorPanelFacingDataSource()
	:r2::edit::InspectorPanelComponentDataSource("Facing Component", 0, 0)
	,mnoptrEditor(nullptr)
	,mInspectorPanel(nullptr)
{

}

InspectorPanelFacingDataSource::InspectorPanelFacingDataSource(r2::Editor* noptrEditor, r2::ecs::ECSCoordinator* coordinator, const r2::edit::InspectorPanel* inspectorPanel)
	: r2::edit::InspectorPanelComponentDataSource("Facing Component", coordinator->GetComponentType<FacingComponent>(), coordinator->GetComponentTypeHash<FacingComponent>())
	, mnoptrEditor(noptrEditor)
	, mInspectorPanel(inspectorPanel)
{

}

void InspectorPanelFacingDataSource::DrawComponentData(void* componentData, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	FacingComponent* facingComponentPtr = static_cast<FacingComponent*>(componentData);
	FacingComponent& facingComponent = *facingComponentPtr;

	glm::vec3 originalFacing = facingComponent.facing;

	ImGui::Text("Facing Direction: ");
	ImGui::SameLine();
	bool changed = false;

	ImGui::PushItemWidth(80.0f);
	if (ImGui::DragFloat("##label facingdirx", &facingComponent.facing.x, 2, -1.0, 1.0))
	{
		facingComponent.facing.y = 0;

		if (glm::length(facingComponent.facing) == 0)
		{
			facingComponent.facing.x = originalFacing.x;
		}
		changed = true;
	}

	ImGui::SameLine();
	if (ImGui::DragFloat("##label facingdiry", &facingComponent.facing.y, 2, -1.0, 1.0))
	{
		facingComponent.facing.x = 0;

		if (r2::math::NearZero(glm::length(facingComponent.facing)))
		{
			facingComponent.facing.y = originalFacing.y;
		}

		changed = true;
	}
	ImGui::PopItemWidth();

	facingComponent.facing = glm::normalize(facingComponent.facing);

	//snap to cardinal axis
	if (glm::abs(facingComponent.facing.x) > glm::abs(facingComponent.facing.y))
	{
		facingComponent.facing = glm::sign(facingComponent.facing.x) * glm::vec3(1, 0, 0);
	}
	else
	{
		facingComponent.facing = glm::sign(facingComponent.facing.y) * glm::vec3(0, 1, 0);
	}
	
	//also draw the facing vector
	r2::ecs::TransformComponent* transformComponent = coordinator->GetComponentPtr<r2::ecs::TransformComponent>(theEntity);

	if (transformComponent)
	{
		r2::draw::renderer::DrawArrow(transformComponent->accumTransform.position + glm::vec3(0, 0, 1), facingComponent.facing, 1.0, 0.3, glm::vec4(1, 1, 0, 1), true);

		if (changed)
		{
			if (glm::length(facingComponent.facing) != 0)
			{
				glm::vec3 eulerAngles = glm::eulerAngles(transformComponent->localTransform.rotation);
				float diffAngle = glm::angle(originalFacing, facingComponent.facing);

				glm::vec3 axis = glm::cross(originalFacing, facingComponent.facing);
				if (r2::math::NearZero(glm::length(axis)))
				{
					axis = glm::vec3(0, 0, 1);
				}
				transformComponent->localTransform.rotation = glm::angleAxis(diffAngle, axis) * transformComponent->localTransform.rotation;

				r2::ecs::TransformDirtyComponent transformDirty;
				transformDirty.dirtyFlags = r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;
				coordinator->AddComponentIfNeeded<r2::ecs::TransformDirtyComponent>(theEntity, transformDirty);
			}
		}
	}
}

bool InspectorPanelFacingDataSource::InstancesEnabled() const
{
	return false;
}

u32 InspectorPanelFacingDataSource::GetNumInstances(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity) const
{
	return 0;
}

void* InspectorPanelFacingDataSource::GetComponentData(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	return &coordinator->GetComponent<FacingComponent>(theEntity);
}

void* InspectorPanelFacingDataSource::GetInstancedComponentData(u32 i, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	return nullptr;
}

void InspectorPanelFacingDataSource::DeleteComponent(r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{
	coordinator->RemoveComponent<FacingComponent>(theEntity);
}

void InspectorPanelFacingDataSource::DeleteInstance(u32 i, r2::ecs::ECSCoordinator* coordinator, r2::ecs::Entity theEntity)
{

}

void InspectorPanelFacingDataSource::AddComponent(r2::ecs::ECSCoordinator* coordinator,r2::ecs::Entity theEntity)
{
	FacingComponent newFacingComponent;
	newFacingComponent.facing = glm::vec3(0, -1, 0);

	coordinator->AddComponent<FacingComponent>(theEntity, newFacingComponent);
}

void InspectorPanelFacingDataSource::AddNewInstances(r2::ecs::ECSCoordinator* coordinator,r2::ecs::Entity theEntity, u32 numInstances)
{

}



#endif