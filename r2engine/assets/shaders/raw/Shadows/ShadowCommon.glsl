#ifndef GLSL_SHADOWS_COMMON
#define GLSL_SHADOWS_COMMON

#include "Common/Texture.glsl"

vec2 VogelDiskSample(int sampleIndex, int samplesCount, float phi)
{
	float GOLDEN_ANGLE = 2.4;

	float r = sqrt(sampleIndex + 0.5) / sqrt(samplesCount);
	float theta = sampleIndex * GOLDEN_ANGLE + phi;
	
	return vec2(r * cos(theta), r * sin(theta));
}

float InterleavedGradientNoise(vec2 screenPosition)
{
	vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
  	return fract(magic.z * fract(dot(screenPosition, magic.xy)));
}

float Sample3DShadowMapDepth(CubemapAddress shadowSurface, vec3 pos, vec3 offset, float page)
{
	//vec4 coord = vec4(pos + offset, page);
	//@TODO(Serge): another tricky case
	float shadowSample = SampleCubemapPageRGBA(shadowSurface, pos + offset, page).r;//texture(samplerCubeArray(shadowSurface.container), coord).r;
	return shadowSample;
}

float SampleShadowMapDepth(Tex2DAddress shadowSurface, vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, float page)
{
	vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
	
	float shadowSample = SampleTextureWithPage(shadowSurface, uv, page).r;

	return shadowSample;
}

float SampleShadowMap(Tex2DAddress shadowSurface, vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, float page, float depth)
{
	float shadowSample = SampleShadowMapDepth(shadowSurface, base_uv, u, v, shadowMapSizeInv, page);
	return depth > shadowSample ? 1.0 : 0.0;
}

#endif