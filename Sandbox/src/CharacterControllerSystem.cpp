#include "CharacterControllerSystem.h"
#include "r2/Core/Input/IInputGather.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "PlayerCommandComponent.h"
#include "FacingComponent.h"
#include "r2/Game/ECS/Components/AnimationTransitionComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include "MoveUpdateComponent.h"
#include "GridPositionComponent.h"
#include "GameUtils.h"

s32 GetAnimationClipIndexForAssetName(const char* animationName, const r2::draw::Model* model)
{
	s32 result = -1;
	const auto numAnimationClips = r2::sarr::Size(*model->optrAnimationClips);

	for (u32 i = 0; i < numAnimationClips; ++i)
	{
		const auto* animationClip = r2::sarr::At(*model->optrAnimationClips, i);

		if (animationClip->mAssetName.hashID == STRING_ID(animationName))
		{
			return static_cast<s32>(i);
		}
	}

	return result;
}


glm::vec3 GetFacingFromPlayerCommandComponent(const PlayerCommandComponent& playerCommandComponent, const glm::vec3& originalFacing)
{
	glm::vec3 newFacing;

	//we're moving and not grabbing
	if (playerCommandComponent.downAction.isEnabled && !(playerCommandComponent.upAction.isEnabled || playerCommandComponent.leftAction.isEnabled || playerCommandComponent.rightAction.isEnabled))
	{
		newFacing = glm::vec3(0, -1, 0);
	}
	else if (playerCommandComponent.upAction.isEnabled && !(playerCommandComponent.downAction.isEnabled || playerCommandComponent.leftAction.isEnabled || playerCommandComponent.rightAction.isEnabled))
	{
		newFacing = glm::vec3(0, 1, 0);
	}
	else if (playerCommandComponent.leftAction.isEnabled && !(playerCommandComponent.upAction.isEnabled || playerCommandComponent.downAction.isEnabled || playerCommandComponent.rightAction.isEnabled))
	{
		newFacing = glm::vec3(-1, 0, 0);
	}
	else if (playerCommandComponent.rightAction.isEnabled && !(playerCommandComponent.upAction.isEnabled || playerCommandComponent.leftAction.isEnabled || playerCommandComponent.downAction.isEnabled))
	{
		newFacing = glm::vec3(1, 0, 0);
	}
	else
	{
		newFacing = originalFacing;
	}

	return newFacing;
}


eMovementType GetMovementType(const glm::vec3& facing)
{
	if (facing == glm::vec3(1, 0, 0))
	{
		return MOVE_RIGHT;
	}
	else if (facing == glm::vec3(-1, 0, 0))
	{
		return MOVE_LEFT;
	}
	else if (facing == glm::vec3(0, 1, 0))
	{
		return MOVE_UP;
	}
	else if (facing == glm::vec3(0, -1, 0))
	{
		return MOVE_DOWN;
	}

	R2_CHECK(false, "Not supported!");
	return MOVE_LEFT;
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

CharacterControllerSystem::CharacterControllerSystem()
{
	mKeepSorted = false;
}

void CharacterControllerSystem::Update()
{
	const auto numPlayerEntities = r2::sarr::Size(*mEntities);

	for (u32 i = 0; i < numPlayerEntities; ++i)
	{
		r2::ecs::Entity e = r2::sarr::At(*mEntities, i);

		//@NOTE(Serge): very temporary 
		const MoveUpdateComponent* moveUpdateComponentPtr = mnoptrCoordinator->GetComponentPtr<MoveUpdateComponent>(e);
		const PlayerCommandComponent& playerCommandComponent = mnoptrCoordinator->GetComponent<PlayerCommandComponent>(e);
		r2::ecs::TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<r2::ecs::TransformComponent>(e);
		GridPositionComponent& gridPositionComponent = mnoptrCoordinator->GetComponent<GridPositionComponent>(e);

		bool shouldStopEarly = (moveUpdateComponentPtr && !moveUpdateComponentPtr->hasArrived && (moveUpdateComponentPtr->gridMovementCurrentTime <= 50) && !(
			playerCommandComponent.downAction.isEnabled ||
			playerCommandComponent.upAction.isEnabled ||
			playerCommandComponent.leftAction.isEnabled ||
			playerCommandComponent.rightAction.isEnabled));

		if (shouldStopEarly)
		{
			//snap to the starting grid position 
			transformComponent.localTransform.position = utils::CalculateWorldPositionFromGridPosition(moveUpdateComponentPtr->startingGridPosition, gridPositionComponent.pivotOffset);

			r2::ecs::TransformDirtyComponent transformDirtyComponent;
			transformDirtyComponent.dirtyFlags = r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;
			mnoptrCoordinator->AddComponentIfNeeded(e, transformDirtyComponent);
			gridPositionComponent.localGridPosition = moveUpdateComponentPtr->startingGridPosition;

			mnoptrCoordinator->RemoveComponent<MoveUpdateComponent>(e);
			continue;
		}
		else if ((moveUpdateComponentPtr && moveUpdateComponentPtr->hasArrived))
		{
			//remove the MoveUpdateComponent
			mnoptrCoordinator->RemoveComponent<MoveUpdateComponent>(e);
		}
		else if(moveUpdateComponentPtr && !moveUpdateComponentPtr->hasArrived)
		{
			continue;
		}
		
		const r2::ecs::SkeletalAnimationComponent& skeletonAnimationComponent = mnoptrCoordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(e);
		
		const FacingComponent& facingComponent = mnoptrCoordinator->GetComponent<FacingComponent>(e);


		glm::vec3 newFacing = facingComponent.facing;
		s32 animationClipIndex = skeletonAnimationComponent.currentAnimationIndex;
		bool shouldLoop = false;
		bool shouldMove = false;
		eMovementType movementType = MOVE_NONE;
		float movementSpeed = 0.0f;

		if (!playerCommandComponent.downAction.isEnabled &&
			!playerCommandComponent.upAction.isEnabled &&
			!playerCommandComponent.leftAction.isEnabled &&
			!playerCommandComponent.rightAction.isEnabled &&
			!playerCommandComponent.grabAction.isEnabled)
		{
			//we're in idle
			animationClipIndex = GetAnimationClipIndexForAssetName("idle", skeletonAnimationComponent.animModel);
			shouldLoop = true;
		}
		else if (!playerCommandComponent.grabAction.isEnabled &&(
			playerCommandComponent.downAction.isEnabled ||
			playerCommandComponent.upAction.isEnabled ||
			playerCommandComponent.leftAction.isEnabled ||
			playerCommandComponent.rightAction.isEnabled))
		{
			newFacing = GetFacingFromPlayerCommandComponent(playerCommandComponent, newFacing);

			animationClipIndex = GetAnimationClipIndexForAssetName("slowrun", skeletonAnimationComponent.animModel);
			shouldLoop = true;
			movementSpeed = 2.0f;

			movementType = GetMovementType(newFacing);
		}
		else if (playerCommandComponent.grabAction.isEnabled && !(
			playerCommandComponent.downAction.isEnabled ||
			playerCommandComponent.upAction.isEnabled ||
			playerCommandComponent.leftAction.isEnabled ||
			playerCommandComponent.rightAction.isEnabled))
		{
			//grabbing and not moving

			//@TODO(Serge): need more context to make this work correctly but for now just testing

			animationClipIndex = GetAnimationClipIndexForAssetName("startpull", skeletonAnimationComponent.animModel);

		}
		else if (playerCommandComponent.grabAction.isEnabled && (
			playerCommandComponent.downAction.isEnabled ||
			playerCommandComponent.upAction.isEnabled ||
			playerCommandComponent.leftAction.isEnabled ||
			playerCommandComponent.rightAction.isEnabled))
		{
			//grabbing and moving
			newFacing = GetFacingFromPlayerCommandComponent(playerCommandComponent, newFacing);
			animationClipIndex = GetAnimationClipIndexForAssetName("push", skeletonAnimationComponent.animModel);
			shouldLoop = true;
			movementSpeed = 0.75f;
			movementType = GetMovementType(newFacing);
		}

#ifdef R2_EDITOR
		if (animationClipIndex == -1)
		{
			continue;
		}
#endif

		R2_CHECK(animationClipIndex != -1, "Should never happen");

		//add the transition if necessary
		if (animationClipIndex != skeletonAnimationComponent.currentAnimationIndex)
		{
			r2::ecs::AnimationTransitionComponent animationTransitionComponent;
			animationTransitionComponent.currentAnimationIndex = skeletonAnimationComponent.currentAnimationIndex;
			animationTransitionComponent.nextAnimationIndex = animationClipIndex;
			animationTransitionComponent.timeToTransition = 0.0; //@TODO(Serge): right now we're making this instant - but later we should have this info in a character file of some kind
			animationTransitionComponent.currentTransitionTime = 0.0; //should always start at 0
			animationTransitionComponent.loop = shouldLoop;

			if (mnoptrCoordinator->HasComponent<r2::ecs::AnimationTransitionComponent>(e))
			{
				r2::ecs::AnimationTransitionComponent& currentTransitionComponent = mnoptrCoordinator->GetComponent<r2::ecs::AnimationTransitionComponent>(e);

				//Replace the transition for now - should think of the proper way to do this
				currentTransitionComponent = animationTransitionComponent;
			}
			else
			{
				mnoptrCoordinator->AddComponent<r2::ecs::AnimationTransitionComponent>(e, animationTransitionComponent);
			}
		}
		
		//@TODO(Serge): add the move update component

		if (movementType != MOVE_NONE)
		{
			MoveUpdateComponent moveUpdateComponent;

			moveUpdateComponent.gridMovementSpeed = movementSpeed;
			moveUpdateComponent.gridMovementCurrentTime = 0.0f;
			moveUpdateComponent.gridMovementTotalMovementTimeNeeded = r2::util::SecondsToMilliseconds(1.0f / moveUpdateComponent.gridMovementSpeed);
			moveUpdateComponent.oldFacing = facingComponent.facing;
			moveUpdateComponent.newFacing = newFacing;
			moveUpdateComponent.facingTransitionTime = 0.0f;
			moveUpdateComponent.hasArrived = false;
			moveUpdateComponent.startingGridPosition = gridPositionComponent.localGridPosition;
			moveUpdateComponent.endGridPosition = GetGridMoveOffset(movementType) + moveUpdateComponent.startingGridPosition;
			moveUpdateComponent.movementType = movementType;

			mnoptrCoordinator->AddComponent<MoveUpdateComponent>(e, moveUpdateComponent);
		}

	
	}

	//Remove all of the PlayerCommandComponents from the entities
	if (numPlayerEntities > 0)
	{
		r2::SArray<r2::ecs::Entity>* entitiesCopy = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::ecs::Entity, numPlayerEntities);

		r2::sarr::Copy(*entitiesCopy, *mEntities);

		for (u32 i = 0; i < numPlayerEntities; ++i)
		{
			//remove the dirty flags
			r2::ecs::Entity e = r2::sarr::At(*entitiesCopy, i);
			mnoptrCoordinator->RemoveComponent<PlayerCommandComponent>(e);
		}

		FREE(entitiesCopy, *MEM_ENG_SCRATCH_PTR);
	}

}
