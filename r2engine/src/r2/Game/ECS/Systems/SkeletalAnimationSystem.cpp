#include "r2pch.h"

#include "r2/Game/ECS/Systems/SkeletalAnimationSystem.h"
#include "r2/Render/Animation/AnimationPlayer.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/AnimationTransitionComponent.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Render/Animation/Pose.h"

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
		auto deltaTime = r2::util::MillisecondsToSecondsF32(static_cast<float>(CPLAT.TickRate()));

		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);
			SkeletalAnimationComponent& animationComponent = mnoptrCoordinator->GetComponent<SkeletalAnimationComponent>(e);
			InstanceComponentT<SkeletalAnimationComponent>* instancedAnimationComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<SkeletalAnimationComponent>>(e);

			AnimationTransitionComponent* animationTransitionComponent = mnoptrCoordinator->GetComponentPtr<AnimationTransitionComponent>(e);


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
			const u32 numShaderBones = r2::anim::pose::Size(*animationComponent.animModel->animSkeleton.mRestPose);
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

			u32 offset = 0;

			if (animationComponent.currentAnimationIndex >= 0)
			{
				const r2::anim::AnimationClip* clip = r2::sarr::At(*animationComponent.animModel->optrAnimationClips, animationComponent.currentAnimationIndex);

				//@TODO(Serge): Right now we're just hard transitioning to the new animation. We need to implement the real crossfading 
				if (animationTransitionComponent && animationTransitionComponent->nextAnimationIndex != animationComponent.currentAnimationIndex)
				{

					clip = r2::sarr::At(*animationComponent.animModel->optrAnimationClips, animationTransitionComponent->nextAnimationIndex);
					animationComponent.animationTime = 0.0;
					animationComponent.shouldLoop = animationTransitionComponent->loop;
					animationComponent.currentAnimationIndex = animationTransitionComponent->nextAnimationIndex;

					//@TODO(Serge): the character controller might actually be the one to remove this
					mnoptrCoordinator->RemoveComponent<r2::ecs::AnimationTransitionComponent>(e);
				}

				animationComponent.animationTime = r2::anim::PlayAnimationClip(*clip, *animationComponent.animationPose, deltaTime + animationComponent.animationTime, animationComponent.shouldLoop);
			}
			
			r2::anim::pose::GetMatrixPalette(*animationComponent.animationPose, animationComponent.animModel->animSkeleton, animationComponent.shaderBones, offset);

#ifdef R2_DEBUG
			if (debugBonesToUse)
			{
				r2::sarr::Clear(*debugBonesToUse);
				r2::anim::pose::GetDebugBones(*animationComponent.animationPose, debugBonesToUse);
			}
#endif
			if (instancedAnimationComponent && !animationComponent.shouldUseSameTransformsForAllInstances)
			{
				for (u32 j = 0; j < instancedAnimationComponent->numInstances; ++j)
				{
					SkeletalAnimationComponent& skeletalAnimationComponent = r2::sarr::At(*instancedAnimationComponent->instances, j);

					r2::sarr::Clear(*skeletalAnimationComponent.shaderBones);

					R2_CHECK(r2::sarr::Capacity(*skeletalAnimationComponent.shaderBones) == numShaderBones, "We don't have enough space in order to animate all of the instances of this anim model");

					if (skeletalAnimationComponent.currentAnimationIndex >= 0)
					{
						const r2::anim::AnimationClip* clip = r2::sarr::At(*skeletalAnimationComponent.animModel->optrAnimationClips, skeletalAnimationComponent.currentAnimationIndex);
						skeletalAnimationComponent.animationTime = r2::anim::PlayAnimationClip(*clip, *skeletalAnimationComponent.animationPose, deltaTime + animationComponent.animationTime, skeletalAnimationComponent.shouldLoop);
					}

					r2::anim::pose::GetMatrixPalette(*skeletalAnimationComponent.animationPose, skeletalAnimationComponent.animModel->animSkeleton, skeletalAnimationComponent.shaderBones, offset);

#ifdef R2_DEBUG
					if (instancedDebugBoneComponent && j < instancedDebugBoneComponent->numInstances)
					{
						//@TODO(Serge): fix when we add a new skeletal animation instance

						DebugBoneComponent& debugBonesComponent = r2::sarr::At(*instancedDebugBoneComponent->instances, j);

						debugBonesToUse = debugBonesComponent.debugBones;
						r2::sarr::Clear(*debugBonesToUse);

						r2::anim::pose::GetDebugBones(*skeletalAnimationComponent.animationPose, debugBonesToUse);
					}
					else
					{
						debugBonesToUse = nullptr;
					}
#endif
				}
			}

#ifdef R2_DEBUG
			if (debugBoneComponent && instancedDebugBoneComponent && animationComponent.shouldUseSameTransformsForAllInstances)
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