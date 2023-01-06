#ifndef __RENDER_PASS_H__
#define __RENDER_PASS_H__

#include "r2/Utils/Utils.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Renderer/RenderTarget.h"
#include "r2/Core/Memory/Memory.h"



namespace r2::draw
{
	enum RenderPassType: s8
	{
		RPT_NONE = -1,
		
		RPT_GBUFFER,
		RPT_SHADOWS,
		RPT_FINAL_COMPOSITE,
		RPT_ZPREPASS,
		RPT_POINTLIGHT_SHADOWS,
		RPT_AMBIENT_OCCLUSION,
		RPT_AMBIENT_OCCLUSION_DENOISE,
		RPT_AMBIENT_OCCLUSION_TEMPORAL_DENOISE,
		RPT_ZPREPASS_SHADOWS,
		RPT_CLUSTERS,
		RPT_SSR,
		RPT_SMAA_EDGE_DETECTION,
		RPT_SMAA_BLENDING_WEIGHT,
		RPT_SMAA_NEIGHBORHOOD_BLENDING,

		RPT_OUTPUT,
		NUM_RENDER_PASSES
	};

	struct RenderPassConfig
	{
		PrimitiveType primitiveType;
		s32 flags = 0;
	};

	struct RenderPass
	{
		RenderPassType type;
		RenderPassConfig config;

		RenderTargetSurface renderInputTargetHandles[NUM_RENDER_TARGET_SURFACES];
		u32 numRenderInputTargets;

		RenderTargetSurface renderOutputTargetHandle;
	};
}

namespace r2::draw::rp
{
	template <class ARENA>
	RenderPass* CreateRenderPass(ARENA& arena, RenderPassType type, const RenderPassConfig& config, const std::initializer_list<RenderTargetSurface>& renderInputTargets, RenderTargetSurface renderOutputTarget, const char* file, s32 line, const char* description)
	{
		R2_CHECK(renderInputTargets.size() <= NUM_RENDER_TARGET_SURFACES, "Added too many render input targets to the render pass! Trying to add %d targets but only %d are allowed!", renderInputTargets.size(), NUM_RENDER_TARGET_SURFACES);

		RenderPass* pass = ALLOC_VERBOSE(RenderPass, arena, file, line, description);

		pass->type = type;
		pass->config = config;
		pass->numRenderInputTargets = renderInputTargets.size();
		
		pass->renderOutputTargetHandle = renderOutputTarget;

		u32 i = 0;
		for (const auto& targetHandle : renderInputTargets)
		{
			pass->renderInputTargetHandles[i] = targetHandle;
			++i;
		}

		return pass;
	}

	template <class ARENA>
	void DestroyRenderPass(ARENA& arena, RenderPass* renderPass)
	{
		FREE(renderPass, arena);
	}
}

#endif
