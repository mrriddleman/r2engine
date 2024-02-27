#include "CharacterControllerSystem.h"
#include "r2/Core/Input/IInputGather.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "PlayerCommandComponent.h"
#include "FacingComponent.h"
#include "r2/Game/ECS/Components/AnimationTransitionComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/TransformDirtyComponent.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "r2/Core/Math/MathUtils.h"

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

CharacterControllerSystem::CharacterControllerSystem()
{

}

void CharacterControllerSystem::Update()
{
	const auto numPlayerEntities = r2::sarr::Size(*mEntities);

	for (u32 i = 0; i < numPlayerEntities; ++i)
	{
		r2::ecs::Entity e = r2::sarr::At(*mEntities, i);
		const PlayerCommandComponent& playerCommandComponent = mnoptrCoordinator->GetComponent<PlayerCommandComponent>(e);
		const r2::ecs::SkeletalAnimationComponent& skeletonAnimationComponent = mnoptrCoordinator->GetComponent<r2::ecs::SkeletalAnimationComponent>(e);
		r2::ecs::TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<r2::ecs::TransformComponent>(e);
		FacingComponent& facingComponent = mnoptrCoordinator->GetComponent<FacingComponent>(e);

		glm::vec3 newFacing = facingComponent.facing;
		s32 animationClipIndex = skeletonAnimationComponent.currentAnimationIndex;
		bool shouldLoop = false;

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

			animationClipIndex = GetAnimationClipIndexForAssetName("walking", skeletonAnimationComponent.animModel);
			shouldLoop = true;
		}
		else if (playerCommandComponent.grabAction.isEnabled && !(
			playerCommandComponent.downAction.isEnabled ||
			playerCommandComponent.upAction.isEnabled ||
			playerCommandComponent.leftAction.isEnabled ||
			playerCommandComponent.rightAction.isEnabled))
		{
			//grabbing and not moving

			//@TODO(Serge): need more context to make this work correctly but for now just testing

			animationClipIndex = GetAnimationClipIndexForAssetName("pushstart", skeletonAnimationComponent.animModel);

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
		}

#ifdef R2_EDITOR
		if (animationClipIndex == -1)
		{
			continue;
		}
#endif

		R2_CHECK(animationClipIndex != -1, "Should never happen");

		float angle = glm::angle(facingComponent.facing, newFacing);
		glm::vec3 axis = glm::cross(facingComponent.facing, newFacing);
		if (r2::math::NearZero(glm::length(axis)  ))
		{
			axis = glm::vec3(0, 0, 1);
		}

		glm::quat quaternionDiff = glm::normalize( glm::angleAxis(angle, axis) );
		transformComponent.localTransform.rotation = glm::normalize( quaternionDiff * transformComponent.localTransform.rotation );
		
		facingComponent.facing = newFacing;

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
		
		//@TODO(Serge): add the move update as well - for now just testing the animation transitions
		//				for right now add a TransformationDirtyComponent to make up for it

		//@TODO(Serge): move this to the movement system
		r2::ecs::TransformDirtyComponent transformDirtyComponent;
		transformDirtyComponent.dirtyFlags = r2::ecs::eTransformDirtyFlags::LOCAL_TRANSFORM_DIRTY;

		mnoptrCoordinator->AddComponentIfNeeded(e, transformDirtyComponent);

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
