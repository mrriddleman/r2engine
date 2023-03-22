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
			const InstanceComponent* instanceComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponent>(e);

			if (!instanceComponent)
			{
				r2::draw::renderer::DrawDebugBones(*debugBoneComponent.debugBones, transformComponent.modelMatrix, debugBoneComponent.color);
			}
			else
			{
				const SkeletalAnimationComponent& animationComponent = mnoptrCoordinator->GetComponent<SkeletalAnimationComponent>(e);

				const u32 numModels = r2::sarr::Size(*instanceComponent->instanceModels);
				const u32 numBonesPerInstance = r2::sarr::Size(*animationComponent.animModel->boneInfo);

				r2::SArray<glm::mat4>* modelMats = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, glm::mat4, numModels + 1);
				r2::sarr::Push(*modelMats, transformComponent.modelMatrix);
				r2::sarr::Append(*modelMats, *instanceComponent->instanceModels);

				r2::SArray<u64>* numBonesPerModel = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u64, numModels + 1);

				r2::sarr::Fill(*numBonesPerModel, (u64)numBonesPerInstance);

				r2::draw::renderer::DrawDebugBones(*debugBoneComponent.debugBones, *numBonesPerModel, *modelMats, debugBoneComponent.color);

				FREE(numBonesPerModel, *MEM_ENG_SCRATCH_PTR);
				FREE(modelMats, *MEM_ENG_SCRATCH_PTR);
					
			}
		}
	}

}

#endif