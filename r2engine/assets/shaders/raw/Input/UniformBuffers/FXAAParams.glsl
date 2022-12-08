#ifndef GLSL_FXAA_PARAMS
#define GLSL_FXAA_PARAMS

#include "Common/Texture.glsl"

layout (std140, binding=6) uniform FXAAParams
{
	Tex2DAddress fxaa_inputTexture;
	float fxaa_lumaThreshold;
	float fxaa_lumaMulReduce;
	float fxaa_lumaMinReduce;
	float fxaa_maxSpan;
	vec2 fxaa_texelStep;
};


#endif