#ifndef GLSL_SSR_PARAMS
#define GLSL_SSR_PARAMS

#include "Common/Texture.glsl"

layout (std140, binding = 4) uniform SSRParams
{
	float ssr_stride;
	float ssr_zThickness;
	int ssr_rayMarchIterations;
	float ssr_strideZCutoff;
	
	Tex2DAddress ssr_ditherTexture;
	
	float ssr_ditherTilingFactor;
	int ssr_roughnessMips;
	int ssr_coneTracingSteps;
	float ssr_maxFadeDistance;
	
	float ssr_fadeScreenStart;
	float ssr_fadeScreenEnd;
	float ssr_maxDistance;
};

#endif