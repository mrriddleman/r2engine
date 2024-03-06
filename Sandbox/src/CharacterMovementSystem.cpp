#include "CharacterMovementSystem.h"
#include "FacingComponent.h"
#include "GridPositionComponent.h"
#include "MoveUpdateComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "r2/Game/ECS/Components/HierarchyComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "r2/Core/Math/MathUtils.h"

#include "r2/Platform/Platform.h"
#include "r2/Core/Engine.h"
#include "GameUtils.h"
#include "r2/Render/Renderer/Renderer.h"

glm::quat GetQuaternionDiffFromFacingComponent(const glm::vec3& oldFacing, const glm::vec3& newFacing)
{
	float angle = glm::angle(oldFacing, newFacing);
	glm::vec3 axis = glm::cross(oldFacing, newFacing);
	if (r2::math::NearZero(glm::length(axis)))
	{
		axis = glm::vec3(0, 0, 1);
	}

	return glm::normalize(glm::angleAxis(angle, axis));
}

void UpdateFacingComponent(const MoveUpdateComponent& moveUpdateComponent, const f64 tickRateMilli, r2::ecs::TransformComponent& transformComponent, FacingComponent& facingComponent)
{
	//@TODO(Serge): tween between the the old facing and the new facing
	glm::quat quaternionDiff = GetQuaternionDiffFromFacingComponent(moveUpdateComponent.oldFacing, moveUpdateComponent.newFacing);
	transformComponent.localTransform.rotation = glm::normalize(quaternionDiff * transformComponent.localTransform.rotation);
}

namespace
{
	glm::ivec3 GetGridMoveOffset(eMovementType movementType)
	{
		switch (movementType)
		{
		case MOVE_LEFT:
			return glm::ivec3(-1, 0, 0);
			break;
		case MOVE_RIGHT:
			return glm::ivec3(1, 0, 0);
			break;
		case MOVE_UP:
			return glm::ivec3(0, 1, 0);
			break;
		case MOVE_DOWN:
			return glm::ivec3(0, -1, 0);
			break;
		case MOVE_NONE:
			return glm::ivec3(0);
			break;
		default:
			R2_CHECK(false, "Unsupported movement type!");
			break;
		}

		return glm::ivec3(0);
	}
}

CharacterMovementSystem::CharacterMovementSystem()
{
	mKeepSorted = true;
}

void CharacterMovementSystem::Update()
{
	const auto numEntitiesToMove = r2::sarr::Size(*mEntities);
	const auto tickRateMilli = CPLAT.TickRate();
	const auto tickRateSeconds = r2::util::MillisecondsToSecondsF32(tickRateMilli);

	for (u32 i = 0; i < numEntitiesToMove; ++i)
	{
		r2::ecs::Entity e = r2::sarr::At(*mEntities, i);
		MoveUpdateComponent& moveUpdateComponent = mnoptrCoordinator->GetComponent<MoveUpdateComponent>(e);
		
		if (moveUpdateComponent.hasArrived)
		{
			continue;
		}

		GridPositionComponent& gridPositionComponent = mnoptrCoordinator->GetComponent<GridPositionComponent>(e);
		r2::ecs::TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<r2::ecs::TransformComponent>(e);
		FacingComponent& facingComponent = mnoptrCoordinator->GetComponent<FacingComponent>(e);

		glm::ivec3 gridMoveOffset = GetGridMoveOffset(moveUpdateComponent.movementType);

		//This is for the internal game position + facing
		//move the grid position immediately and set new facing - only once
		if (gridPositionComponent.localGridPosition != moveUpdateComponent.endGridPosition)
		{
			UpdateFacingComponent(moveUpdateComponent, tickRateMilli, transformComponent, facingComponent);

			facingComponent.facing = moveUpdateComponent.newFacing;
			gridPositionComponent.localGridPosition += gridMoveOffset;

			const r2::ecs::HierarchyComponent* hierarchyComponent = mnoptrCoordinator->GetComponentPtr<r2::ecs::HierarchyComponent>(e);

			if (hierarchyComponent && hierarchyComponent->parent != r2::ecs::INVALID_ENTITY)
			{
				const GridPositionComponent& parentGridPositionComponent = mnoptrCoordinator->GetComponent<GridPositionComponent>(hierarchyComponent->parent);
				gridPositionComponent.globalGridPosition = parentGridPositionComponent.globalGridPosition + gridPositionComponent.localGridPosition;
			}
			else
			{
				gridPositionComponent.globalGridPosition = gridPositionComponent.localGridPosition; 
			}
		}

		//Now update the visual position

		glm::vec3 worldOffset = utils::CalculateWorldPositionFromGridPosition(gridMoveOffset, gridPositionComponent.pivotOffset);

		//@TODO(Serge): acceleration/deceleration

		if (moveUpdateComponent.gridMovementCurrentTime + tickRateMilli <= moveUpdateComponent.gridMovementTotalMovementTimeNeeded)
		{
			transformComponent.localTransform.position += worldOffset * moveUpdateComponent.gridMovementSpeed * tickRateSeconds;
			moveUpdateComponent.gridMovementCurrentTime += tickRateMilli;

			if (r2::math::NearEq(moveUpdateComponent.gridMovementCurrentTime, moveUpdateComponent.gridMovementTotalMovementTimeNeeded))
			{
				moveUpdateComponent.hasArrived = true;
				moveUpdateComponent.gridMovementCurrentTime = moveUpdateComponent.gridMovementTotalMovementTimeNeeded;

				transformComponent.localTransform.position = utils::CalculateWorldPositionFromGridPosition(moveUpdateComponent.endGridPosition, gridPositionComponent.pivotOffset);
			}
		}
		else
		{
			moveUpdateComponent.gridMovementCurrentTime = moveUpdateComponent.gridMovementTotalMovementTimeNeeded;
			moveUpdateComponent.hasArrived = true;

			transformComponent.localTransform.position = utils::CalculateWorldPositionFromGridPosition(moveUpdateComponent.endGridPosition, gridPositionComponent.pivotOffset);
		}

		r2::ecs::TransformDirtyComponent transformDirtyComponent;
		transformDirtyComponent.dirtyFlags = r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;
		mnoptrCoordinator->AddComponentIfNeeded(e, transformDirtyComponent);

	}


}

void CharacterMovementSystem::Render()
{
#ifdef R2_DEBUG
	const auto numEntitiesToMove = r2::sarr::Size(*mEntities);

	for (u32 i = 0; i < numEntitiesToMove; ++i)
	{
		r2::ecs::Entity e = r2::sarr::At(*mEntities, i);
		const GridPositionComponent& gridPositionComponent = mnoptrCoordinator->GetComponent<GridPositionComponent>(e);

		glm::vec3 worldPosition = utils::CalculateWorldPositionFromGridPosition(gridPositionComponent.globalGridPosition, gridPositionComponent.pivotOffset);
		r2::draw::renderer::DrawCube(worldPosition, 0.25, glm::vec4(1, 0, 0, 1), true);

	}
	
#endif
}