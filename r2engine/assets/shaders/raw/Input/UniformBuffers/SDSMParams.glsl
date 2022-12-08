#ifndef GLSL_SDSM_PARAMS
#define GLSL_SDSM_PARAMS

#include "Common/Texture.glsl"

layout (std140, binding = 3) uniform SDSMParams
{
	vec4 lightSpaceBorder;
	vec4 maxScale;
	vec4 projMultSplitScaleZMultLambda;
	float dilationFactor;
	uint scatterTileDim;
	uint reduceTileDim;
	uint padding;
	vec4 splitScaleMultFadeFactor;
	Tex2DAddress blueNoiseTexture;
};


#endif