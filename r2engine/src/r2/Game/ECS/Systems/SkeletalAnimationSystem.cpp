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
			InstanceComponentT<SkeletalAnimationComponent>* instancedAnimationComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<SkeletalAnimationComponent>>(e);

			r2::SArray<r2::draw::DebugBone>* debugBonesToUse = nullptr;
#ifdef R2_DEBUG
			DebugBoneComponent* debugBoneComponent = mnoptrCoordinator->GetComponentPtr<DebugBoneComponent>(e);
			InstanceComponentT<DebugBoneComponent>* instancedDebugBoneComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<DebugBoneComponent>>(e);

			if (debugBoneComponent)
			{
				R2_CHECK(debugBoneComponent->debugBones != nullptr, "These should not be null");
				debugBonesToUse = debugBoneComponent->debugBones;
			}
#endif

			R2_CHECK(animationComponent.shaderBones != nullptr, "These should not be null");

			u32 numInstancesToAnimate = 1;
			const u32 numShaderBones = r2::sarr::Size(*animationComponent.animModel->optrBoneInfo);
			u32 totalNumShaderBones = numShaderBones;

			if (instancedAnimationComponent)
			{
				const u32 numInstances = instancedAnimationComponent->numInstances;
				if (!animationComponent.shouldUseSameTransformsForAllInstances)
				{
					numInstancesToAnimate += numInstances;
					totalNumShaderBones += (numInstances * numShaderBones);
				}
			}

			R2_CHECK(r2::sarr::Capacity(*animationComponent.shaderBones) == numShaderBones, "We don't have enough space in order to animate all of the instances of this anim model");

			r2::sarr::Clear(*animationComponent.shaderBones);
#ifdef R2_DEBUG
			if (debugBonesToUse)
			{
				r2::sarr::Clear(*debugBonesToUse);
			}
#endif
			const auto ticks = CENG.GetTicks();
			
			u32 offset = 0;

			r2::draw::PlayAnimationForAnimModel(
				ticks,
				animationComponent.startTime,
				(bool)animationComponent.shouldLoop,
				*animationComponent.animModel,
				r2::sarr::At(*animationComponent.animModel->optrAnimations, animationComponent.currentAnimationIndex),
				*animationComponent.shaderBones, debugBonesToUse, 0);

			if (instancedAnimationComponent && !animationComponent.shouldUseSameTransformsForAllInstances)
			{
				for (u32 j = 0; j < instancedAnimationComponent->numInstances; ++j)
				{
					SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(*instancedAnimationComponent->instances, j);
#ifdef R2_DEBUG
					if (instancedDebugBoneComponent)
					{
						//@TODO(Serge): fix when we add a new skeletal animation instance

						DebugBoneComponent& debugBonesComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, j);

						debugBonesToUse = debugBonesComponent.debugBones;
						r2::sarr::Clear(*debugBonesToUse);
					}
					else
					{
						debugBonesToUse = nullptr;
					}
#endif
					r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

					R2_CHECK(r2::sarr::Capacity(*skeletalAnimationComponent.shaderBones) == numShaderBones, "We don't have enough space in order to animate all of the instances of this anim model");

					r2::draw::PlayAnimationForAnimModel(
						ticks,
						skeletalAnimationComponent.startTime,
						(bool)skeletalAnimationComponent.shouldLoop,
						*skeletalAnimationComponent.animModel,
						r2::sarr::At(*skeletalAnimationComponent.animModel->optrAnimations, skeletalAnimationComponent.currentAnimationIndex),
						*skeletalAnimationComponent.shaderBones, debugBonesToUse, 0);
				}
			}

#ifdef R2_DEBUG
			if (instancedDebugBoneComponent && animationComponent.shouldUseSameTransformsForAllInstances)
			{
				//copy all of the bone data into the instances
				for (u32 i = 0; i < instancedDebugBoneComponent->numInstances; ++i)
				{
					DebugBoneComponent& nextDebugBoneComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, i);
					r2::sarr::Copy(*nextDebugBoneComponent.debugBones, *debugBoneComponent->debugBones);
				}
			}
#endif
			
		}
	}
}