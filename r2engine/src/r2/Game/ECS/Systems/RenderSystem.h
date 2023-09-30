#ifndef __RENDER_SYSTEM_H__
#define __RENDER_SYSTEM_H__

#include "r2/Game/ECS/System.h"

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"

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
			r2::SArray<glm::mat4>* transforms;

			r2::SArray<r2::draw::RenderMaterialParams>* renderMaterialParams;
			r2::SArray<r2::draw::ShaderHandle>* shaderHandles;

			r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms;

			static u64 MemorySize(u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemoryProperties& memorySizeStruct);

		};

		RenderSystem();
		~RenderSystem();

		template<class ARENA>
		bool Init(ARENA& arena, u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms)
		{
			r2::mem::utils::MemoryProperties memorySizeStruct;
			memorySizeStruct.headerSize = arena.HeaderSize();
			memorySizeStruct.alignment = 16;

			memorySizeStruct.boundsChecking = 0;
#ifdef R2_DEBUG
			memorySizeStruct.boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
			
			u64 memorySize = MemorySize(maxNumInstancesPerModel, maxNumMaterialsPerModel, maxNumShaderBoneTransforms, memorySizeStruct);

			mMemoryBoundary = MAKE_BOUNDARY(arena, memorySize, memorySizeStruct.alignment);
			
			R2_CHECK(mMemoryBoundary.location != nullptr, "Couldn't create the memory boundary");

			mArena = EMPLACE_STACK_ARENA_IN_BOUNDARY(mMemoryBoundary);

			R2_CHECK(mArena != nullptr, "We couldn't emplace the stack arena");

			mBatch.transforms = MAKE_SARRAY(*mArena, glm::mat4, maxNumInstancesPerModel);
			mBatch.renderMaterialParams = MAKE_SARRAY(*mArena, r2::draw::RenderMaterialParams, maxNumMaterialsPerModel);
			mBatch.shaderHandles = MAKE_SARRAY(*mArena, r2::draw::ShaderHandle, maxNumMaterialsPerModel);
			mBatch.boneTransforms = MAKE_SARRAY(*mArena, r2::draw::ShaderBoneTransform, maxNumShaderBoneTransforms);

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

		static u64 MemorySize(u32 maxNumInstancesPerModel, u32 maxNumMaterialsPerModel, u32 maxNumShaderBoneTransforms, const r2::mem::utils::MemoryProperties& memorySize);
	private:

		void DrawRenderComponent(
			ecs::Entity entity,
			const TransformComponent& transform,
			const RenderComponent& renderComponent,
			const SkeletalAnimationComponent* animationComponent,
			const InstanceComponentT<TransformComponent>* instancedTransformComponent,
			const InstanceComponentT<SkeletalAnimationComponent>* instancedSkeletalAnimationComponent);
		
		void ClearPerFrameData();

		r2::mem::utils::MemBoundary mMemoryBoundary;
		r2::mem::StackArena* mArena;

		RenderSystemGatherBatch mBatch;
	};
}

#endif // __RENDER_SYSTEM_H__
