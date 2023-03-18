#ifndef __RENDER_SYSTEM_H__
#define __RENDER_SYSTEM_H__

#include "r2/Game/ECS/System.h"

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Model/Material.h"

namespace r2::ecs
{
	class RenderSystem : public System
	{
	public:

		struct RenderSystemGatherBatch
		{
			r2::draw::DrawParameters drawParams;
			r2::draw::PrimitiveType primitiveType;

			r2::SArray<r2::draw::vb::GPUModelRefHandle>* modelRefs;
			r2::SArray<glm::mat4>* transforms;
			r2::SArray<u32>* instances;
			r2::SArray<r2::draw::MaterialHandle>* materialHandles;
			r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms;

			static u64 MemorySize(u32 maxNumModels, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemorySizeStruct& memorySizeStruct);

		};

		RenderSystem();
		~RenderSystem();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel)
		{
			r2::mem::utils::MemorySizeStruct memorySizeStruct;
			memorySizeStruct.headerSize = arena.HeaderSize();
			memorySizeStruct.alignment = 16;

			memorySizeStruct.boundsChecking = = 0;
#ifdef R2_DEBUG
			memorySizeStruct.boundsChecking = = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
			
			u64 memorySize = MemorySize(maxNumStaticModelsToDraw, maxNumAnimModelsToDraw, maxNumMaterialsPerModel, maxNumBoneTransformsPerAnimModel, memorySizeStruct);

			mMemoryBoundary = MAKE_BOUNDARY(arena, memorySize, memorySizeStruct.alignment);
			
			R2_CHECK(mMemoryBoundary.location != nullptr, "Couldn't create the memory boundary");

			mArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(mMemoryBoundary);

			R2_CHECK(mArena != nullptr, "We couldn't emplace the stack arena");

			mStaticBatches = MAKE_SHASHMAP(*mArena, r2::SArray<RenderSystemGatherBatch>*, maxNumStaticModelsToDraw * r2::SHashMap<RenderSystemGatherBatch>::LoadFactorMultiplier());

			R2_CHECK(mStaticBatches != nullptr, "We couldn't create the mStaticBatches");

			mDynamicBatches = MAKE_SHASHMAP(*mArena, r2::SArray<RenderSystemGatherBatch>*, maxNumAnimModelsToDraw * r2::SHashMap<RenderSystemGatherBatch>::LoadFactorMultiplier());

			R2_CHECK(mDynamicBatches != nullptr, "We couldn't create the mDynamicBatches");

			mResetPointPtr = mArena->StartPtr() + mArena->TotalBytesAllocated();

			return true;
		}

		void Render();

		template<class ARENA>
		void Shutdown(ARENA& arena)
		{
			RESET_ARENA(*mArena);
			FREE_EMPLACED_ARENA(mArena);
			FREE(mMemoryBoundary.location, arena);
		}

		static u64 MemorySize(u32 maxNumStaticModelsToDraw, u32 maxNumAnimModelsToDraw, u32 maxNumMaterialsPerModel, u32 maxNumBoneTransformsPerAnimModel, const r2::mem::utils::MemorySizeStruct& memorySize);
	private:

		using GatherBatchPtr = r2::SHashMap<r2::SArray<RenderSystemGatherBatch>*>*;

		r2::mem::utils::MemBoundary mMemoryBoundary;
		r2::mem::StackArena* mArena;
		void* mResetPointPtr;

		GatherBatchPtr mStaticBatches;
		GatherBatchPtr mDynamicBatches;
	};
}

#endif // __RENDER_SYSTEM_H__
