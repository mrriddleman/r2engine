#include "r2pch.h"

#include "r2/Game/ECS/Systems/RenderSystem.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Utils/Hash.h"

namespace r2::ecs
{
	RenderSystem::RenderSystem()
		: mMemoryBoundary{}
		, mArena(nullptr)
		, mPerFrameArena(nullptr)
		, mMaxNumStaticModelsToDraw(0)
		, mMaxNumAnimModelsToDraw(0)
		, mMaxNumMaterialsPerModel(0)
		, mMaxNumBoneTransformsPerAnimModel(0)
		, mMaxNumInstancesPerModel(0)
		, mStaticBatches(nullptr)
		, mDynamicBatches(nullptr)
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
			const InstanceComponent* instanceComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponent>(e);

			GatherBatchPtr gatherBatchToUse = mStaticBatches;
			u32 maxNumModels = mMaxNumStaticModelsToDraw;
			if (animationComponent)
			{
				gatherBatchToUse = mDynamicBatches;
				maxNumModels = mMaxNumAnimModelsToDraw;
			}

			AddComponentsToGatherBatch(gatherBatchToUse, maxNumModels, transformComponent, renderComponent, animationComponent, instanceComponent);
		}


		SubmitBatch(mStaticBatches);
		SubmitBatch(mDynamicBatches);
		

		//Do the simple thing to start
		FreeAllPerFrameData();
	}

	void RenderSystem::AddComponentsToGatherBatch(GatherBatchPtr gatherBatchPtr, u32 maxNumModelsToCreate, const TransformComponent& transform, const RenderComponent& renderComponent, const SkeletalAnimationComponent* animationComponent, const InstanceComponent* instanceComponent)
	{
		//I think the idea here is to gather all of the similar state together and only call the necessary amount of DrawModel/DrawModels of the renderer
		//We have to break things up on certain criteria:
		//1. Different DrawParameters
		//2. Animated vs Not animated
		//3. Material overrides
		//4. Primitive types

		//first generate the render key
		bool hasMaterialOverrides = renderComponent.optrOverrideMaterials != nullptr;
		bool isAnimated = animationComponent != nullptr;

		draw::key::RenderSystemKey renderKey = r2::draw::key::GenerateRenderSystemKey(hasMaterialOverrides, renderComponent.primitiveType, utils::HashBytes32(&renderComponent, sizeof(renderComponent.drawParameters)));

		//now find or create the RenderSystemGatherBatch that we should add to 

		RenderSystemGatherBatch* defaultGatherBatch = nullptr;

		RenderSystemGatherBatch* gatherBatch = r2::shashmap::Get(*gatherBatchPtr, renderKey.keyValue, defaultGatherBatch);

		if (gatherBatch == defaultGatherBatch)
		{
			//need to make a gather batch for this
			gatherBatch = ALLOC(RenderSystemGatherBatch, *mPerFrameArena);

			gatherBatch->modelRefHandles = MAKE_SARRAY(*mPerFrameArena, r2::draw::vb::GPUModelRefHandle, maxNumModelsToCreate);
			gatherBatch->transforms = MAKE_SARRAY(*mPerFrameArena, glm::mat4, maxNumModelsToCreate * mMaxNumInstancesPerModel);
			gatherBatch->instances = MAKE_SARRAY(*mPerFrameArena, u32, maxNumModelsToCreate);
			gatherBatch->materialHandles = nullptr;
			if (hasMaterialOverrides)
			{
				gatherBatch->materialHandles = MAKE_SARRAY(*mPerFrameArena, r2::draw::MaterialHandle, mMaxNumMaterialsPerModel * maxNumModelsToCreate);
			}
			gatherBatch->boneTransforms = nullptr;
			if (isAnimated)
			{
				gatherBatch->boneTransforms = MAKE_SARRAY(*mPerFrameArena, r2::draw::ShaderBoneTransform, mMaxNumBoneTransformsPerAnimModel * maxNumModelsToCreate);
			}
			
			gatherBatch->drawParams = renderComponent.drawParameters;
			gatherBatch->primitiveType = renderComponent.primitiveType;

			r2::shashmap::Set(*gatherBatchPtr, renderKey.keyValue, gatherBatch);
		}

		//now add in the data to gatherBatch
		r2::sarr::Push(*gatherBatch->modelRefHandles, renderComponent.gpuModelRefHandle);
		
		r2::sarr::Push(*gatherBatch->transforms, transform.modelMatrix);
		u32 numInstances = 1;

		if (instanceComponent)
		{
			numInstances += r2::sarr::Size(*instanceComponent->offsets);
			r2::sarr::Append(*gatherBatch->transforms, *instanceComponent->instanceModels);
		}

		r2::sarr::Push(*gatherBatch->instances, numInstances);
		
		if (hasMaterialOverrides)
		{
			r2::sarr::Append(*gatherBatch->materialHandles, *renderComponent.optrOverrideMaterials);
		}
		if (isAnimated)
		{
			r2::sarr::Append(*gatherBatch->boneTransforms, *animationComponent->shaderBones);
		}
	}

	void RenderSystem::SubmitBatch(GatherBatchPtr gatherBatch)
	{
		auto batchIter = r2::shashmap::Begin(*gatherBatch);

		for (; batchIter != r2::shashmap::End(*gatherBatch); ++batchIter)
		{
			const auto batch = batchIter->value;
			r2::draw::renderer::DrawModels(batch->drawParams, *batch->modelRefHandles, *batch->transforms, *batch->instances, batch->materialHandles, batch->boneTransforms);
		}
	}

	void RenderSystem::FreeAllPerFrameData()
	{
		r2::shashmap::Clear(*mStaticBatches);
		r2::shashmap::Clear(*mDynamicBatches);

		RESET_ARENA(*mPerFrameArena);
	}

	void RenderSystem::ClearPerFrameData(GatherBatchPtr gatherBatch)
	{
		auto batchIter = r2::shashmap::Begin(*gatherBatch);

		for (; batchIter != r2::shashmap::End(*gatherBatch); ++batchIter)
		{
			auto batch = batchIter->value;
			r2::sarr::Clear(*batch->modelRefHandles);
			r2::sarr::Clear(*batch->instances);
			r2::sarr::Clear(*batch->transforms);
			if (batch->materialHandles)
			{
				r2::sarr::Clear(*batch->materialHandles);
			}
			if (batch->boneTransforms)
			{
				r2::sarr::Clear(*batch->boneTransforms);
			}
		}
	}

	u64 RenderSystem::MemorySize(u32 maxNumStaticBatches, u32 maxNumDynamicBatches, u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel, const r2::mem::utils::MemorySizeStruct& memorySizeStruct)
	{
		u64 memorySize = 0;
		
		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderSystem), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * 2 +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<RenderSystemGatherBatch*>::MemorySize(maxNumStaticBatches * r2::SHashMap<u64>::LoadFactorMultiplier()), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<RenderSystemGatherBatch*>::MemorySize(maxNumDynamicBatches * r2::SHashMap<u64>::LoadFactorMultiplier()), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			MemorySizeForPerFrameArena(maxNumStaticBatches, maxNumDynamicBatches, maxNumStaticModelsToDraw, maxNumAnimModelsToDraw, maxNumInstancesPerModel, maxNumMaterialsPerModel, maxNumBoneTransformsPerAnimModel, memorySizeStruct);
		
		R2_CHECK(memorySize <= Megabytes(32), "Don't let this go crazy");

		return memorySize;
	}

	u64 RenderSystem::MemorySizeForPerFrameArena(u32 maxNumStaticBatches, u32 maxNumDynamicBatches, u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel, const r2::mem::utils::MemorySizeStruct& memorySizeStruct)
	{
		u64 memorySize = 0;

		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderSystemGatherBatch), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumStaticBatches;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderSystemGatherBatch), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumDynamicBatches;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(RenderSystemGatherBatch::MemorySize(maxNumStaticModelsToDraw / maxNumStaticBatches, maxNumInstancesPerModel, maxNumMaterialsPerModel, 0, memorySizeStruct), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumStaticBatches;
		memorySize += r2::mem::utils::GetMaxMemoryForAllocation(RenderSystemGatherBatch::MemorySize(maxNumAnimModelsToDraw / maxNumDynamicBatches, maxNumInstancesPerModel, maxNumMaterialsPerModel, maxNumBoneTransformsPerAnimModel, memorySizeStruct), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) * maxNumDynamicBatches;

		return memorySize;
	}

	u64 RenderSystem::RenderSystemGatherBatch::MemorySize(u32 maxNumModels, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel , u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemorySizeStruct& memorySizeStruct)
	{
		u64 memorySize = 0;

		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(RenderSystemGatherBatch), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::vb::GPUModelRefHandle>::MemorySize(maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(maxNumModels * maxNumInstancesPerModel), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u32>::MemorySize(maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::MaterialHandle>::MemorySize(maxNumMaterialsPerModel * maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			//@NOTE: this is inadvertently correct - pretty sure we can only have this many bones anyways (shaderwise)
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderBoneTransform>::MemorySize(maxNumShaderBoneTransforms * maxNumModels), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking);

		return memorySize;
	}

}