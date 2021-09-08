#ifndef __RENDER_PASS_H__
#define __RENDER_PASS_H__

#include "r2/Utils/Utils.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Renderer/RenderTarget.h"
#include "r2/Core/Memory/Memory.h"
#include <bitset>

namespace r2::draw
{

	//using RenderPa

	enum RenderPassType: u8
	{
		RPT_GBUFFER = 0,
		RPT_FINAL_COMPOSITE,
		NUM_RENDER_PASSES
	};

	//enum CommandBucketType
	//{
	//	CBT_PRE_RENDER = 0,

	//	//@NOTE(Serge): These should mirror RenderPassType
	//	CBT_GBUFFER,
	//	CBT_FINAL_COMPOSITE,

	//	CBT_POST_RENDER,
	//	NUM_COMMAND_BUCKET_TYPES
	//};

	const u32 MAX_RENDER_TARGETS_FOR_RENDER_PASS = 4;

	struct RenderPassConfig
	{
		PrimitiveType primitiveType;
		s32 flags = 0;
	};

	struct RenderPass
	{
		RenderPassType type;
		RenderPassConfig config;

		RenderTargetSurface renderInputTargetHandles[MAX_RENDER_TARGETS_FOR_RENDER_PASS];
		u32 numRenderInputTargets;

		RenderTargetSurface renderOutputTargetHandles[MAX_RENDER_TARGETS_FOR_RENDER_PASS];
		u32 numRenderOutputTargets;
	};
}

namespace r2::draw::rp
{
	template <class ARENA>
	RenderPass* CreateRenderPass(ARENA& arena, RenderPassType type, const RenderPassConfig& config, const std::initializer_list<RenderTargetSurface>& renderInputTargets, const std::initializer_list<RenderTargetSurface>& renderOutputTargets, const char* file, s32 line, const char* description)
	{
		R2_CHECK(renderOutputTargets.size() <= MAX_RENDER_TARGETS_FOR_RENDER_PASS, "Added too many render output targets to the render pass! Trying to add %d targets but only %d are allowed!", renderOutputTargets.size(), MAX_RENDER_TARGETS_FOR_RENDER_PASS);
		R2_CHECK(renderInputTargets.size() <= MAX_RENDER_TARGETS_FOR_RENDER_PASS, "Added too many render input targets to the render pass! Trying to add %d targets but only %d are allowed!", renderInputTargets.size(), MAX_RENDER_TARGETS_FOR_RENDER_PASS);

		RenderPass* pass = ALLOC_VERBOSE(RenderPass, arena, file, line, description);

		pass->type = type;
		pass->config = config;
		pass->numRenderInputTargets = renderInputTargets.size();
		pass->numRenderOutputTargets = renderOutputTargets.size();

		u32 i = 0;
		for (const auto& targetHandle : renderOutputTargets)
		{
			pass->renderOutputTargetHandles[i] = targetHandle;
			++i;
		}

		i = 0;
		for (const auto& targetHandle : renderInputTargets)
		{
			pass->renderInputTargetHandles[i] = targetHandle;
			++i;
		}
	}

	template <class ARENA>
	void DestroyRenderPass(ARENA& arena, RenderPass* renderPass)
	{
		FREE(renderPass, arena);
	}
}

#endif
