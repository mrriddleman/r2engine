#include "r2pch.h"

#include "r2/Game/ECS/Systems/RenderSystem.h"

#include "r2/Core/Assets/AssetLib.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"
#include "r2/Render/Model/Materials/MaterialParamsPack_generated.h"
#include "r2/Render/Model/Materials/MaterialParamsPackHelpers.h"
#include "r2/Render/Renderer/ShaderSystem.h"
#include "r2/Render/Renderer/Renderer.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Utils/Hash.h"



namespace r2::ecs
{
	RenderSystem::RenderSystem()
		: mMemoryBoundary{}
		, mArena(nullptr)
		, mBatch{}
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

			const InstanceComponentT<TransformComponent>* instancedTransformsComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<TransformComponent>>(e);
			const InstanceComponentT<SkeletalAnimationComponent>* instancedAnimationComponent = mnoptrCoordinator->GetComponentPtr<InstanceComponentT<SkeletalAnimationComponent>>(e);

			DrawRenderComponent(transformComponent, renderComponent, animationComponent, instancedTransformsComponent, instancedAnimationComponent);

			ClearPerFrameData();
		}
	}

	void RenderSystem::DrawRenderComponent(
		const TransformComponent& transform,
		const RenderComponent& renderComponent,
		const SkeletalAnimationComponent* animationComponent,
		const InstanceComponentT<TransformComponent>* instancedTransformComponent,
		const InstanceComponentT<SkeletalAnimationComponent>* instancedSkeletalAnimationComponent)
	{

		bool hasMaterialOverrides = renderComponent.optrMaterialOverrideNames != nullptr && r2::sarr::Size(*renderComponent.optrMaterialOverrideNames) > 0;
		bool isAnimated = animationComponent != nullptr;

		//@TEMPORARY so that we can remove the material system calls from the Renderer itself
		r2::sarr::Push(*mBatch.transforms, transform.modelMatrix);
		u32 numInstances = 1;

		if (instancedTransformComponent)
		{
			numInstances += instancedTransformComponent->numInstances;
			for (size_t i = 0; i < instancedTransformComponent->numInstances; i++)
			{
				r2::sarr::Push(*mBatch.transforms, r2::sarr::At(*instancedTransformComponent->instances, i).modelMatrix);
			}
		}

		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();

		if (hasMaterialOverrides)
		{
			u32 numOverrides = r2::sarr::Size(*renderComponent.optrMaterialOverrideNames);

			r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();

			for (u32 i = 0; i < numOverrides; ++i)
			{
				const r2::mat::MaterialName& materialName = r2::sarr::At(*renderComponent.optrMaterialOverrideNames, i);

				const r2::draw::RenderMaterialParams* renderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, materialName.name);
				R2_CHECK(renderMaterial != nullptr, "Can't be nullptr");
				const r2::draw::RenderMaterialParams& nextRenderMaterial = *renderMaterial;

				const byte* manifestData = r2::asset::lib::GetManifestData(assetLib, materialName.packName);

				const flat::MaterialParamsPack* materialManifest = flat::GetMaterialParamsPack(manifestData);

				r2::draw::ShaderHandle materialShaderHandle = r2::draw::shadersystem::FindShaderHandle(r2::mat::GetShaderNameForMaterialName(materialManifest, materialName.name));

				r2::sarr::Push(*mBatch.renderMaterialParams, nextRenderMaterial);
				r2::sarr::Push(*mBatch.shaderHandles, materialShaderHandle);
			}
		}
		else
		{
			//@TODO(Serge): fill with the regular stuff

			const r2::draw::vb::GPUModelRef* gpuModelRef = r2::draw::renderer::GetGPUModelRef(renderComponent.gpuModelRefHandle);

			const u32 numMaterialHandles = gpuModelRef->numMaterials;

			for (u32 i = 0; i < numMaterialHandles; ++i)
			{
				r2::draw::ShaderHandle materialShaderHandle = r2::sarr::At(*gpuModelRef->shaderHandles, i);

				r2::draw::RenderMaterialCache* renderMaterialCache = r2::draw::renderer::GetRenderMaterialCache();


				const r2::draw::RenderMaterialParams* nextRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, r2::sarr::At(*gpuModelRef->renderMaterialHandles, i));

				R2_CHECK(nextRenderMaterial != nullptr, "This should never be nullptr");

				r2::sarr::Push(*mBatch.renderMaterialParams, *nextRenderMaterial);
				r2::sarr::Push(*mBatch.shaderHandles, materialShaderHandle);
			}
		}

		r2::SArray<r2::draw::ShaderBoneTransform>* shaderBoneTransforms = nullptr;

		if (isAnimated)
		{
			r2::sarr::Append(*mBatch.boneTransforms, *animationComponent->shaderBones);

			if (instancedSkeletalAnimationComponent && !animationComponent->shouldUseSameTransformsForAllInstances)
			{
				for (u32 i = 0; i < instancedSkeletalAnimationComponent->numInstances; i++)
				{
					SkeletalAnimationComponent& nextAnimationComponent = r2::sarr::At(*instancedSkeletalAnimationComponent->instances, i);
					r2::sarr::Append(*mBatch.boneTransforms, *nextAnimationComponent.shaderBones);
				}
			}

			shaderBoneTransforms = mBatch.boneTransforms;
		}

		r2::draw::renderer::DrawModel(renderComponent.drawParameters, renderComponent.gpuModelRefHandle, *mBatch.transforms, numInstances, *mBatch.renderMaterialParams, *mBatch.shaderHandles, shaderBoneTransforms); 
	}

	void RenderSystem::ClearPerFrameData()
	{

		r2::sarr::Clear(*mBatch.transforms);
		r2::sarr::Clear(*mBatch.renderMaterialParams);
		r2::sarr::Clear(*mBatch.shaderHandles);
		r2::sarr::Clear(*mBatch.boneTransforms);
	}

	u64 RenderSystem::MemorySize(u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemoryProperties& memorySizeStruct)
	{
		u64 memorySize = 0;
		
		memorySize +=
			r2::ecs::ECSCoordinator::MemorySizeOfSystemType<r2::ecs::RenderSystem>(memorySizeStruct) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			RenderSystemGatherBatch::MemorySize(maxNumInstancesPerModel, maxNumMaterialsPerModel, maxNumShaderBoneTransforms, memorySizeStruct);
		
		R2_CHECK(memorySize <= Megabytes(50), "Don't let this go crazy");

		return memorySize;
	}

	u64 RenderSystem::RenderSystemGatherBatch::MemorySize(u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemoryProperties& memorySizeStruct)
	{
		u64 memorySize = 0;

		memorySize +=
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<glm::mat4>::MemorySize(maxNumInstancesPerModel), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::RenderMaterialParams>::MemorySize(maxNumMaterialsPerModel ), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderHandle>::MemorySize(maxNumMaterialsPerModel), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking) +

			//@NOTE: this is inadvertently correct - pretty sure we can only have this many bones anyways (shaderwise)
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::draw::ShaderBoneTransform>::MemorySize(maxNumShaderBoneTransforms), memorySizeStruct.alignment, memorySizeStruct.headerSize, memorySizeStruct.boundsChecking);

		return memorySize;
	}
}