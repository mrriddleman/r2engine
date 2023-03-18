#include "r2pch.h"

#include "r2/Game/ECS/Systems/RenderSystem.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Utils/Hash.h"

namespace r2::ecs
{


	RenderSystem::RenderSystem()
	{
		mKeepSorted = false;
	}

	RenderSystem::~RenderSystem()
	{

	}

	void RenderSystem::Render()
	{
		R2_CHECK(mEntities != nullptr, "This should never happen");
		R2_CHECK(mnoptrCoordinator != nullptr, "This should never happen");

		const auto numEntities = r2::sarr::Size(*mEntities);

		for (u32 i = 0; i < numEntities; ++i)
		{
			ecs::Entity e = r2::sarr::At(*mEntities, i);

			const TransformComponent& transformComponent = mnoptrCoordinator->GetComponent<TransformComponent>(e);
			const RenderComponent& renderComponent = mnoptrCoordinator->GetComponent<RenderComponent>(e);
			const SkeletalAnimationComponent* animationComponent = mnoptrCoordinator->GetComponentPtr<SkeletalAnimationComponent>(e);

			//I think the idea here is to gather all of the similar state together and only call the necessary amount of DrawModel/DrawModels of the renderer
			//We have to break things up on certain criteria:
			//1. Different DrawParameters
			//2. Animated vs Not animated
			//3. Material overrides
			//4. Primitive types

			if (!animationComponent)
			{

			}
			else
			{

			}


		}

	}

	u64 RenderSystem::MemorySize(u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel, const r2::mem::utils::MemorySizeStruct& memorySizeStruct)
	{
		u64 memorySize = 0;
		
		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderSystem), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::SArray<RenderSystemGatherBatch>*>::MemorySize(maxNumStaticModelsToDraw * r2::SHashMap<u64>::LoadFactorMultiplier()), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<r2::SArray<RenderSystemGatherBatch>*>::MemorySize(maxNumAnimModelsToDraw * r2::SHashMap<u64>::LoadFactorMultiplier()), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RenderSystemGatherBatch>::MemorySize(maxNumStaticModelsToDraw), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumStaticModelsToDraw +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<RenderSystemGatherBatch>::MemorySize(maxNumAnimModelsToDraw), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumAnimModelsToDraw +
			//pretty sure this is a wild overestimate since we're multiplying by the maxNumStaticModelsToDraw and maxNumAnimModelsToDraw
			//could be better if we just set RenderSystemGatherBatch::MemorySize(1, maxNumMaterialsPerModel, etc)
			//that way we're preserving the max amount of models but I dunno yet...
			r2::mem::utils::GetMaxMemoryForAllocation(RenderSystemGatherBatch::MemorySize(maxNumStaticModelsToDraw, maxNumMaterialsPerModel, 0, memorySizeStruct), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumStaticModelsToDraw +
			r2::mem::utils::GetMaxMemoryForAllocation(RenderSystemGatherBatch::MemorySize(maxNumAnimModelsToDraw, maxNumMaterialsPerModel, maxNumBoneTransformsPerAnimModel, memorySizeStruct), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumAnimModelsToDraw;


		return memorySize;
	}

	u64 RenderSystem::RenderSystemGatherBatch::MemorySize(u32 maxNumModels, u32 maxNumMaterialsPerModel , u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemorySizeStruct& memorySizeStruct)
	{
		u64 memorySize = 0;

		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderSystemGatherBatch), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::vb::GPUModelRefHandle>::MemorySize(maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u32>::MemorySize(maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::MaterialHandle>::MemorySize(maxNumMaterialsPerModel * maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderBoneTransform>::MemorySize(maxNumShaderBoneTransforms * maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking);

		return memorySize;
	}

}