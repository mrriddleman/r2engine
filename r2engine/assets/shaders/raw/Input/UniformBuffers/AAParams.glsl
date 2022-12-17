#ifndef GLSL_FXAA_PARAMS
#define GLSL_FXAA_PARAMS

#include "Common/Texture.glsl"

layout (std140, binding=6) uniform AAParams
{
	Tex2DAddress inputTexture;
	float fxaa_lumaThreshold;
	float fxaa_lumaMulReduce;
	float fxaa_lumaMinReduce;
	float fxaa_maxSpan;
	vec2 fxaa_texelStep;
	float smaa_threshold;
	int smaa_maxSearchSteps;
	Tex2DAddress smaa_areaTexture;
	Tex2DAddress smaa_searchTexture;
	ivec4 smaa_subSampleIndices;
	int smaa_cornerRounding;
	int smaa_maxSearchStepsDiag;
};


#endif