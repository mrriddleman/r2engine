#include "r2pch.h"

#include "r2/Game/ECS/Systems/RenderSystem.h"

#include "r2/Core/Assets/AssetLib.h"
#include "r2/Game/ECS/ECSCoordinator.h"
#include "r2/Game/ECS/Components/TransformComponent.h"
#include "r2/Game/ECS/Components/RenderComponent.h"
#include "r2/Game/ECS/Components/SkeletalAnimationComponent.h"
#include "r2/Game/ECS/Components/InstanceComponent.h"

#ifdef R2_EDITOR
#include "r2/Game/ECS/Components/SelectionComponent.h"
#endif

#include "r2/Render/Model/Materials/MaterialHelpers.h"

#include "r2/Render/Model/Shader/ShaderSystem.h"
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

#ifdef R2_EDITOR
			bool hasSelectedComponent = mnoptrCoordinator->HasComponent<ecs::SelectionComponent>(e);

			if (hasSelectedComponent)
			{
				const SelectionComponent& selectionComponent = mnoptrCoordinator->GetComponent<ecs::SelectionComponent>(e);
				DrawRenderComponentSelected(e, selectionComponent, transformComponent, renderComponent, animationComponent, instancedTransformsComponent, instancedAnimationComponent);
			}
			else {
#endif

				DrawRenderComponent(e, transformComponent, renderComponent, animationComponent, instancedTransformsComponent, instancedAnimationComponent);

#ifdef R2_EDITOR
			}
#endif // R2_EDITOR


			ClearPerFrameData();
		}
	}

	void RenderSystem::DrawRenderComponent(
		ecs::Entity entity,
		const TransformComponent& transform,
		const RenderComponent& renderComponent,
		const SkeletalAnimationComponent* animationComponent,
		const InstanceComponentT<TransformComponent>* instancedTransformComponent,
		const InstanceComponentT<SkeletalAnimationComponent>* instancedSkeletalAnimationComponent)
	{
		bool hasMaterialOverrides = renderComponent.optrMaterialOverrideNames != nullptr && r2::sarr::Size(*renderComponent.optrMaterialOverrideNames) > 0;
		bool isAnimated = animationComponent != nullptr;

		u32 numInstances = 1;

		if (instancedTransformComponent)
		{
			numInstances += instancedTransformComponent->numInstances;
		}
#ifdef R2_EDITOR
		r2::SArray<s32>* instancesToDraw = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, s32, numInstances);
		r2::sarr::Push(*instancesToDraw, -1);
#endif

		//@TEMPORARY so that we can remove the material system calls from the Renderer itself
		r2::sarr::Push(*mBatch.transforms, transform.modelMatrix);
		

		if (instancedTransformComponent)
		{
			for (size_t i = 0; i < instancedTransformComponent->numInstances; i++)
			{
				const auto& tranformComponent = r2::sarr::At(*instancedTransformComponent->instances, i);
				r2::sarr::Push(*mBatch.transforms, tranformComponent.modelMatrix);
#ifdef R2_EDITOR
				r2::sarr::Push(*instancesToDraw, static_cast<s32>(i));
#endif
			}
		}

		PopulateBatchMaterials(hasMaterialOverrides, renderComponent);


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

#ifdef R2_EDITOR



		r2::draw::renderer::DrawModelEntity(entity, *instancesToDraw, renderComponent.drawParameters, renderComponent.gpuModelRefHandle, *mBatch.transforms, *mBatch.renderMaterialParams, *mBatch.shaderHandles, shaderBoneTransforms);
		FREE(instancesToDraw, *MEM_ENG_SCRATCH_PTR);

#else
		r2::draw::renderer::DrawModel(renderComponent.drawParameters, renderComponent.gpuModelRefHandle, *mBatch.transforms, numInstances, *mBatch.renderMaterialParams, *mBatch.shaderHandles, shaderBoneTransforms);
#endif

		
	}

#ifdef R2_EDITOR
	void RenderSystem::DrawRenderComponentSelected(
		ecs::Entity entity,
		const SelectionComponent& selectionComponent,
		const TransformComponent& transform,
		const RenderComponent& renderComponent,
		const SkeletalAnimationComponent* animationComponent,
		const InstanceComponentT<TransformComponent>* instancedTransformComponent,
		const InstanceComponentT<SkeletalAnimationComponent>* instancedSkeletalAnimationComponent)
	{
		//essentially we need to break up the draw of the entity based on which instances are selected
		//We have to change the state of the selected instances and submit them as a draw
		r2::asset::AssetLib& assetLib = CENG.GetAssetLib();
		
		bool hasMaterialOverrides = renderComponent.optrMaterialOverrideNames != nullptr && r2::sarr::Size(*renderComponent.optrMaterialOverrideNames) > 0;
		
		bool isAnimated = animationComponent != nullptr;

		u32 totalInstances = 1;

		if (instancedTransformComponent)
		{
			totalInstances += r2::sarr::Size(*instancedTransformComponent->instances);
		}

		r2::SArray<r2::draw::ShaderBoneTransform>* shaderBoneTransforms = nullptr;
		u32 numSelectedInstances = r2::sarr::Size(*selectionComponent.selectedInstances);
		s64 hasBaseComponentSelected = -1;

		PopulateBatchMaterials(hasMaterialOverrides, renderComponent);

		if (numSelectedInstances > 0)
		{
			hasBaseComponentSelected = r2::sarr::IndexOf(*selectionComponent.selectedInstances, -1);

			if (hasBaseComponentSelected != -1)
			{
				r2::sarr::Push(*mBatch.transforms, transform.modelMatrix);
			}

			if (instancedTransformComponent)
			{
				for (s32 i = 0; i < numSelectedInstances; ++i)
				{
					s32 selectedInstance = r2::sarr::At(*selectionComponent.selectedInstances, i);
					if (selectedInstance != -1 && selectedInstance < instancedTransformComponent->numInstances)
					{
						r2::sarr::Push(*mBatch.transforms, r2::sarr::At(*instancedTransformComponent->instances, selectedInstance).modelMatrix);
					}
				}
			}

			if (isAnimated)
			{
				if (hasBaseComponentSelected != -1)
				{
					r2::sarr::Append(*mBatch.boneTransforms, *animationComponent->shaderBones);
				}

				if (instancedSkeletalAnimationComponent && !animationComponent->shouldUseSameTransformsForAllInstances)
				{
					for (s32 i = 0; i < numSelectedInstances; ++i)
					{
						s32 selectedInstance = r2::sarr::At(*selectionComponent.selectedInstances, i);
						if (selectedInstance != -1 && selectedInstance < instancedSkeletalAnimationComponent->numInstances)
						{
							SkeletalAnimationComponent& nextAnimationComponent = r2::sarr::At(*instancedSkeletalAnimationComponent->instances, selectedInstance);
							r2::sarr::Append(*mBatch.boneTransforms, *nextAnimationComponent.shaderBones);
						}
					}
				}

				shaderBoneTransforms = mBatch.boneTransforms;
			}

			r2::draw::DrawParameters stencilDrawParams;
			memset(&stencilDrawParams, 0, sizeof(r2::draw::DrawParameters));
			memcpy(&stencilDrawParams, &renderComponent.drawParameters, sizeof(r2::draw::DrawParameters));
			stencilDrawParams.stencilState.op.stencilFail = r2::draw::KEEP;
			stencilDrawParams.stencilState.op.depthFail = r2::draw::KEEP;
			stencilDrawParams.stencilState.op.depthAndStencilPass = r2::draw::REPLACE;
			stencilDrawParams.stencilState.stencilWriteEnabled = true;
			stencilDrawParams.stencilState.stencilEnabled = true;
			stencilDrawParams.stencilState.func.func = r2::draw::ALWAYS;
			stencilDrawParams.stencilState.func.ref = 1;
			stencilDrawParams.stencilState.func.mask = 0xFF;

			r2::draw::renderer::DrawModelEntity(entity, *selectionComponent.selectedInstances, stencilDrawParams, renderComponent.gpuModelRefHandle, *mBatch.transforms, *mBatch.renderMaterialParams, *mBatch.shaderHandles, shaderBoneTransforms);


			//Then we have to submit the outline shader version with its own state 
			stencilDrawParams.flags.Remove(r2::draw::eDrawFlags::DEPTH_TEST);
			stencilDrawParams.layer = r2::draw::DL_EFFECT;
			stencilDrawParams.stencilState.stencilEnabled = true;
			stencilDrawParams.stencilState.stencilWriteEnabled = false;
			stencilDrawParams.stencilState.func.func = r2::draw::NOTEQUAL;
			stencilDrawParams.stencilState.func.ref = 1;
			stencilDrawParams.stencilState.func.mask = 0xFF;

			r2::SArray<r2::draw::RenderMaterialParams>* outlineMaterialParams = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::RenderMaterialParams, 1);
			r2::SArray<r2::draw::ShaderHandle>* outlineShaderHandles = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, r2::draw::ShaderHandle, 1);

			r2::sarr::Push(*outlineMaterialParams, r2::draw::renderer::GetDefaultOutlineRenderMaterialParams(!isAnimated));
			r2::sarr::Push(*outlineShaderHandles, r2::draw::renderer::GetDefaultOutlineShaderHandle(!isAnimated));

			r2::draw::renderer::DrawModelEntity(entity, *selectionComponent.selectedInstances, stencilDrawParams, renderComponent.gpuModelRefHandle, *mBatch.transforms, *outlineMaterialParams, *outlineShaderHandles, shaderBoneTransforms);

			FREE(outlineShaderHandles, *MEM_ENG_SCRATCH_PTR);
			FREE(outlineMaterialParams, *MEM_ENG_SCRATCH_PTR);


		}
		
		if (totalInstances - numSelectedInstances > 0)
		{
			r2::sarr::Clear(*mBatch.transforms);
			r2::sarr::Clear(*mBatch.boneTransforms);
			
			r2::SArray<s32>* instancesToDraw = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, s32, totalInstances);

			//we also have to draw the normal versions separately

			//populate the transforms again with the instances that weren't in the previous batches

			if (hasBaseComponentSelected == -1)
			{
				r2::sarr::Push(*mBatch.transforms, transform.modelMatrix);

				r2::sarr::Push(*instancesToDraw, -1);
			}

			if (instancedTransformComponent)
			{
				for (s32 i = 0; i < instancedTransformComponent->numInstances; ++i)
				{
					if (r2::sarr::IndexOf(*selectionComponent.selectedInstances, i) == -1)
					{
						r2::sarr::Push(*mBatch.transforms, r2::sarr::At(*instancedTransformComponent->instances, i).modelMatrix);

						r2::sarr::Push(*instancesToDraw, i);
					}
				}
			}

			shaderBoneTransforms = nullptr;

			if (isAnimated)
			{
				if (hasBaseComponentSelected == -1)
				{
					r2::sarr::Append(*mBatch.boneTransforms, *animationComponent->shaderBones);
				}

				if (instancedSkeletalAnimationComponent && !animationComponent->shouldUseSameTransformsForAllInstances)
				{
					for (s32 i = 0; i < instancedSkeletalAnimationComponent->numInstances; ++i)
					{
						if (r2::sarr::IndexOf(*selectionComponent.selectedInstances, i) == -1)
						{
							SkeletalAnimationComponent& nextAnimationComponent = r2::sarr::At(*instancedSkeletalAnimationComponent->instances, i);
							r2::sarr::Append(*mBatch.boneTransforms, *nextAnimationComponent.shaderBones);
						}
					}
				}

				shaderBoneTransforms = mBatch.boneTransforms;
			}



			r2::draw::renderer::DrawModelEntity(entity, *instancesToDraw, renderComponent.drawParameters, renderComponent.gpuModelRefHandle, *mBatch.transforms, *mBatch.renderMaterialParams, *mBatch.shaderHandles, shaderBoneTransforms);

			FREE(instancesToDraw, *MEM_ENG_SCRATCH_PTR);
		}
		
	}
#endif

	void RenderSystem::PopulateBatchMaterials(bool hasMaterialOverrides, const RenderComponent& renderComponent)
	{
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

				//const byte* manifestData = r2::asset::lib::GetManifestData(assetLib, materialName.packName);

				//const flat::MaterialPack* materialManifest = flat::GetMaterialPack(manifestData);

				//@TODO(Serge): I'm not sure if this is always correct - shouldn't it depend on the material?
				//				Perhaps we should add something to the material to fix this - for now will work
				r2::draw::eMeshPass meshPass = draw::MP_FORWARD;
				if (renderComponent.drawParameters.layer == draw::DL_TRANSPARENT)
				{
					meshPass = draw::MP_TRANSPARENT; 
				}

				r2::draw::ShaderHandle materialShaderHandle = r2::mat::GetShaderHandleForMaterialName(materialName, draw::MP_FORWARD, renderComponent.isAnimated ? draw::SET_DYNAMIC : draw::SET_STATIC);//r2::draw::shadersystem::FindShaderHandle(r2::mat::GetShaderNameForMaterialName(materialManifest, materialName.name));

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


				const r2::draw::RenderMaterialParams* nextRenderMaterial = r2::draw::rmat::GetGPURenderMaterial(*renderMaterialCache, r2::sarr::At(*gpuModelRef->materialNames, i).name);

				R2_CHECK(nextRenderMaterial != nullptr, "This should never be nullptr");

				r2::sarr::Push(*mBatch.renderMaterialParams, *nextRenderMaterial);
				r2::sarr::Push(*mBatch.shaderHandles, materialShaderHandle);
			}
		}
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