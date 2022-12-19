#ifndef GLSL_SURFACES
#define GLSL_SURFACES

#include "Common/Texture.glsl"

layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
	Tex2DAddress ambientOcclusionDenoiseSurface;
	Tex2DAddress zPrePassShadowsSurface[2];
	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
	Tex2DAddress normalSurface;
	Tex2DAddress specularSurface;
	Tex2DAddress ssrSurface;
	Tex2DAddress convolvedGBUfferSurface[2]; //@TODO(Serge): fix the name
	Tex2DAddress ssrConeTracedSurface;
	Tex2DAddress bloomDownSampledSurface;
	Tex2DAddress bloomBlurSurface;
	Tex2DAddress bloomUpSampledSurface;
	Tex2DAddress smaaEdgeDetectionSurface;
	Tex2DAddress smaaBlendingWeightSurface;
};

#endif