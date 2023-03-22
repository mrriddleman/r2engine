#include "r2pch.h"

#include "r2/Game/ECS/Systems/SkeletalAnimationSystem.h"
#include "r2/Render/Animation/AnimationPlayer.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"



namespace r2::ecs
{

	SkeletalAnimationSystem::SkeletalAnimationSystem()
	{
		mKeepSorted = false;
	}

	SkeletalAnimationSystem::~SkeletalAnimationSystem()
	{

	}

	void SkeletalAnimationSystem::Update()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		const auto numEntities = r2::sarr::Size(*mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);
			SkeletalAnimationComponent& animationComponent = mnoptrCoordinator->GetComponent<SkeletalAnimationComponent>(e);
			InstanceComponent* instanceComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponent>(e);

			//@TODO(Serge): right now the Animation Player requires debug bones to play the animation
			//				we should refactor this in order to remove that requirement
#ifdef R2_DEBUG
			DebugBoneComponent* debugBoneComponent = mnoptrCoordinator->GetComponentPtr<DebugBoneComponent>(e);
			R2_CHECK(debugBoneComponent != nullptr, "Should not be nullptr right now");
			R2_CHECK(debugBoneComponent->debugBones != nullptr, "These should not be null");
#endif

			R2_CHECK(animationComponent.shaderBones != nullptr, "These should not be null");
			R2_CHECK(animationComponent.animationsPerInstance != nullptr, "This should be not null");

			u32 numInstancesToAnimate = 1;
			const u32 numShaderBones = r2::sarr::Size(*animationComponent.animModel->boneInfo);
			u32 totalNumShaderBones = numShaderBones;

			if (instanceComponent)
			{
				const u32 numInstances = r2::sarr::Size(*instanceComponent->offsets);
				if (!animationComponent.shouldUseSameTransformsForAllInstances)
				{
					numInstancesToAnimate += numInstances;
					totalNumShaderBones += (numInstances * numShaderBones);
				}
			}

			R2_CHECK(r2::sarr::Capacity(*animationComponent.shaderBones) >= totalNumShaderBones, "We don't have enough space in order to animate all of the instances of this anim model");
			R2_CHECK(r2::sarr::Size(*animationComponent.animationsPerInstance) == numInstancesToAnimate, "These should be the same");
			R2_CHECK(r2::sarr::Size(*animationComponent.loopPerInstance) == numInstancesToAnimate, "These should be the same");
			R2_CHECK(r2::sarr::Size(*animationComponent.startTimePerInstance) == numInstancesToAnimate, "These should be the same");

			r2::sarr::Clear(*animationComponent.shaderBones);
#ifdef R2_DEBUG
			r2::sarr::Clear(*debugBoneComponent->debugBones);
#endif

			u32 offset = 0;
			for (u32 j = 0; j < numInstancesToAnimate; ++j)
			{
				r2::draw::PlayAnimationForAnimModel(
					CENG.GetTicks(),
					r2::sarr::At(*animationComponent.startTimePerInstance, j),
					r2::sarr::At(*animationComponent.loopPerInstance, j),
					*animationComponent.animModel,
					r2::sarr::At(*animationComponent.animationsPerInstance, j),
					*animationComponent.shaderBones, *debugBoneComponent->debugBones, offset);

				offset += numShaderBones;
			}
		}
		
	}

}