#ifndef __RENDER_SYSTEM_H__
#define __RENDER_SYSTEM_H__

#include "r2/Game/ECS/System.h"

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Model/Materials/Material.h"

namespace r2::ecs
{
	struct TransformComponent;
	struct RenderComponent;
	struct SkeletalAnimationComponent;

	template <typename T> struct InstanceComponentT;


	class RenderSystem : public System
	{
	public:

		struct RenderSystemGatherBatch
		{
			r2::draw::DrawParameters drawParams;
			r2::draw::PrimitiveType primitiveType;

			r2::SArray<r2::draw::vb::GPUModelRefHandle>* modelRefHandles;
			r2::SArray<glm::mat4>* transforms;
			r2::SArray<u32>* instances;
			r2::SArray<r2::draw::MaterialHandle>* materialHandles;
			r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms;

			static u64 MemorySize(u32 maxNumModels, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemoryProperties& memorySizeStruct);

		};

		RenderSystem();
		~RenderSystem();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumberOfStaticBatches, u32 maxNumberOfDynamicBatches, u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel)
		{
			r2::mem::utils::MemoryProperties memorySizeStruct;
			memorySizeStruct.headerSize = arena.HeaderSize();
			memorySizeStruct.alignment = 16;

			memorySizeStruct.boundsChecking = 0;
#ifdef R2_DEBUG
			memorySizeStruct.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
			
			u64 memorySize = MemorySize(maxNumberOfStaticBatches, maxNumberOfDynamicBatches, maxNumStaticModelsToDraw, maxNumAnimModelsToDraw, maxNumInstancesPerModel, maxNumMaterialsPerModel, maxNumBoneTransformsPerAnimModel, memorySizeStruct);

			mMemoryBoundary = MAKE_BOUNDARY(arena, memorySize, memorySizeStruct.alignment);
			
			R2_CHECK(mMemoryBoundary.location != nullptr, "Couldn't create the memory boundary");

			mArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(mMemoryBoundary);

			R2_CHECK(mArena != nullptr, "We couldn't emplace the stack arena");

			mStaticBatches = MAKE_SHASHMAP(*mArena, RenderSystemGatherBatch*, maxNumberOfStaticBatches * r2::SHashMap<RenderSystemGatherBatch*>::LoadFactorMultiplier());

			R2_CHECK(mStaticBatches != nullptr, "We couldn't create the mStaticBatches");

			mDynamicBatches = MAKE_SHASHMAP(*mArena, RenderSystemGatherBatch*, maxNumberOfDynamicBatches * r2::SHashMap<RenderSystemGatherBatch*>::LoadFactorMultiplier());

			R2_CHECK(mDynamicBatches != nullptr, "We couldn't create the mDynamicBatches");

			u64 perFrameMemorySize = MemorySizeForPerFrameArena(maxNumberOfStaticBatches, maxNumberOfDynamicBatches, maxNumStaticModelsToDraw, maxNumAnimModelsToDraw, maxNumInstancesPerModel, maxNumMaterialsPerModel, maxNumBoneTransformsPerAnimModel, memorySizeStruct);
			
			mPerFrameArena = MAKE_STACK_ARENA(*mArena, perFrameMemorySize);

			R2_CHECK(mPerFrameArena != nullptr, "We couldn't create the per frame arena!");

			mMaxNumStaticModelsToDraw = maxNumStaticModelsToDraw;
			mMaxNumAnimModelsToDraw = maxNumAnimModelsToDraw;
			mMaxNumMaterialsPerModel = maxNumMaterialsPerModel;
			mMaxNumBoneTransformsPerAnimModel = maxNumBoneTransformsPerAnimModel;
			mMaxNumInstancesPerModel = maxNumInstancesPerModel;

			return true;
		}

		void Render();

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			RESET_ARENA(*mPerFrameArena);
			RESET_ARENA(*mArena);
			FREE_EMPLACED_ARENA(mArena);
			FREE(mMemoryBoundary.location, arena);
		}

		static u64 MemorySize(u32 maxNumStaticBatches, u32 maxNumDynamicBatches, u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel, const r2::mem::utils::MemoryProperties& memorySize);
	private:

		static u64 MemorySizeForPerFrameArena(u32 maxNumStaticBatches, u32 maxNumDynamicBatches, u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel, const r2::mem::utils::MemoryProperties& memorySize);

		using GatherBatchPtr = r2::SHashMap<RenderSystemGatherBatch*>*;

		void AddComponentsToGatherBatch(
			GatherBatchPtr gatherBatch,
			u32 maxNumModelsToCreate,
			const TransformComponent& transform,
			const RenderComponent& renderComponent,
			const SkeletalAnimationComponent* animationComponent,
			const InstanceComponentT<TransformComponent>* instancedTransformComponent,
			const InstanceComponentT<SkeletalAnimationComponent>* instancedSkeletalAnimationComponent);
		
		void SubmitBatch(GatherBatchPtr gatherBatch);
		void FreeAllPerFrameData();
		void ClearPerFrameData(GatherBatchPtr gatherBatch);

		r2::mem::utils::MemBoundary mMemoryBoundary;
		r2::mem::StackArena* mArena;
		r2::mem::StackArena* mPerFrameArena;

		u32 mMaxNumStaticModelsToDraw;
		u32 mMaxNumAnimModelsToDraw;
		u32 mMaxNumMaterialsPerModel;
		u32 mMaxNumBoneTransformsPerAnimModel;
		u32 mMaxNumInstancesPerModel;

		GatherBatchPtr mStaticBatches;
		GatherBatchPtr mDynamicBatches;
	};
}

#endif // __RENDER_SYSTEM_H__
