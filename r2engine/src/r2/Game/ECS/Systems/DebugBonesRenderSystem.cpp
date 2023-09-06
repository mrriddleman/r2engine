#include "r2pch.h"
#ifdef R2_DEBUG

#include "r2/Game/ECS/Systems/DebugBonesRenderSystem.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Render/Renderer/Renderer.h"

#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::ecs
{
	
	DebugBonesRenderSystem::DebugBonesRenderSystem()
	{
		mKeepSorted = false;
	}

	DebugBonesRenderSystem::~DebugBonesRenderSystem()
	{

	}

	void DebugBonesRenderSystem::Render()
	{
		R2_CHECK(mnoptrCoordinator != nullptr, "Not sure why this should ever be nullptr?");
		R2_CHECK(mEntities != nullptr, "Not sure why this should ever be nullptr?");

		const auto numEntities = r2::sarr::Size(*mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);

			const DebugBoneComponent& debugBoneComponent = mnoptrCoordinator->GetComponent<DebugBoneComponent>(e);
			const TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(e);
			const InstanceComponentT<TransformComponent>* instancedTranformsComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<TransformComponent>>(e);
			const InstanceComponentT<DebugBoneComponent>* instanceDebugBonesComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<DebugBoneComponent>>(e);
			
			r2::draw::renderer::DrawDebugBones(*debugBoneComponent.debugBones, transformComponent.modelMatrix, debugBoneComponent.color);
			
			if(instancedTranformsComponent && instanceDebugBonesComponent)
			{
				const SkeletalAnimationComponent& animationComponent = mnoptrCoordinator->GetComponent<SkeletalAnimationComponent>(e);

				const u32 numModels = instancedTranformsComponent->numInstances;
				const u32 numBonesPerInstance = r2::sarr::Size(*animationComponent.animModel->optrBoneInfo);

				for (u32 j = 0; j < numModels; ++j)
				{
					DebugBoneComponent& debugBoneComponentJ = r2::sarr::At(*instanceDebugBonesComponent->instances, j);
					TransformComponent& transformComponentJ = r2::sarr::At(*instancedTranformsComponent->instances, j);
					r2::draw::renderer::DrawDebugBones(*debugBoneComponentJ.debugBones, transformComponentJ.modelMatrix, debugBoneComponentJ.color);
				}
			}
		}
	}
}

#endif