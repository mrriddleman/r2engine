#include "r2pch.h"
#ifdef R2_DEBUG

#include "r2/Game/ECS/Systems/DebugBonesRenderSystem.h"
#include "r2/Game/ECS/Components/DebugBoneComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Render/Renderer/Renderer.h"
#include "R2/Render/Animation/Pose.h"
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
		GameAssetManager& gameAssetManager = CENG.GetGameAssetManager();

		for (u32 i = 0; i < numEntities; ++i)
		{
			Entity e = r2::sarr::At(*mEntities, i);

			const DebugBoneComponent& debugBoneComponent = mnoptrCoordinator->GetComponent<DebugBoneComponent>(e);
			const TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(e);
			const InstanceComponentT<TransformComponent>* instancedTranformsComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<TransformComponent>>(e);
			const InstanceComponentT<DebugBoneComponent>* instanceDebugBonesComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<DebugBoneComponent>>(e);
			
			const SkeletalAnimationComponent* animationComponent = mnoptrCoordinator->GetComponentPtr<SkeletalAnimationComponent>(e);


			RenderComponent* renderComponent = mnoptrCoordinator->GetComponentPtr<RenderComponent>(e);
			const r2::draw::Model* model = nullptr;

			glm::mat4 transformMatrix = transformComponent.modelMatrix;
			if (renderComponent)
			{

				r2::asset::Asset modelAsset = r2::asset::Asset(renderComponent->assetModelName, r2::asset::RMODEL);

				//@NOTE(Serge): hopefully this never actually loads anything
				r2::draw::ModelHandle modelHandle = gameAssetManager.LoadAsset(modelAsset);

				model = gameAssetManager.GetAssetDataConst<r2::draw::Model>(modelHandle);
				//@PERFORMANCE(Serge): this might be too slow when it comes to moving characters around
				transformMatrix = transformMatrix * model->globalInverseTransform;
			}

			r2::draw::renderer::DrawDebugBones(*debugBoneComponent.debugBones, transformMatrix, debugBoneComponent.color);
			
			if(instancedTranformsComponent && instanceDebugBonesComponent && animationComponent)
			{
				//@NOTE(Serge): alternatively we could do the min?
				auto numInstancesToUse = glm::min(instanceDebugBonesComponent->numInstances, instancedTranformsComponent->numInstances);
				//R2_CHECK(instanceDebugBonesComponent->numInstances <= instancedTranformsComponent->numInstances, "Should always be the case");

				const u32 numModels = numInstancesToUse;//instanceDebugBonesComponent->numInstances;
				const u32 numBonesPerInstance = r2::anim::pose::Size(*animationComponent->animModel->animSkeleton.mRestPose);

				for (u32 j = 0; j < numModels; ++j)
				{
					DebugBoneComponent& debugBoneComponentJ = r2::sarr::At(*instanceDebugBonesComponent->instances, j);
					TransformComponent& transformComponentJ = r2::sarr::At(*instancedTranformsComponent->instances, j);
					glm::mat4 transformMatrix = transformComponentJ.modelMatrix;
					if (model)
					{
						transformMatrix *= model->globalInverseTransform;
					}

					r2::draw::renderer::DrawDebugBones(*debugBoneComponentJ.debugBones, transformMatrix, debugBoneComponentJ.color);
				}
			}
		}
	}
}

#endif