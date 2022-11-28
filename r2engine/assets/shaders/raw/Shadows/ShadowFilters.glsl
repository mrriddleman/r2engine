#ifndef GLSL_SHADOW_FILTERS
#define GLSL_SHADOW_FILTERS

#include "Shadows/ShadowCommon.glsl"

const float NUM_SOFT_SHADOW_SAMPLES = 16.0;
const float SHADOW_FILTER_MAX_SIZE = 0.0035f;
const float PENUMBRA_FILTER_SCALE = 2.4f;


float AvgBlockersDepthToPenumbra(float depth, float avgBlockersDepth)
{
	float penumbra = (depth - avgBlockersDepth) / avgBlockersDepth;
	penumbra *= penumbra;
	return clamp(penumbra , 0.0, 1.0);
}

float Penumbra(Tex2DAddress shadowSurface, float gradientNoise, vec2 shadowMapUV, float depth, int samplesCount, vec2 shadowMapSizeInv, float shadowMapPage)
{
	float avgBlockerDepth = 0.0;
	float blockersCount = 0.0;

	for(int i = 0; i < samplesCount; ++i)
	{
		vec2 sampleUV = VogelDiskSample(i, samplesCount, gradientNoise);
		sampleUV = shadowMapUV + PENUMBRA_FILTER_SCALE * sampleUV;

		float sampleDepth = SampleShadowMap(shadowSurface, sampleUV, 0, 0, shadowMapSizeInv, shadowMapPage, depth);

		if(sampleDepth < depth)
		{
			avgBlockerDepth += sampleDepth;
			blockersCount += 1.0;
		}
	}

	if(blockersCount > 0.0)
	{
		avgBlockerDepth /= blockersCount;
		//return AvgBlockersDepthToPenumbra(depth, avgBlockerDepth);
	}

	return AvgBlockersDepthToPenumbra(depth, avgBlockerDepth);
	//return 0;
}

float SoftShadow(Tex2DAddress shadowTexture, vec3 shadowPosition, vec2 shadowMapSize, float shadowMapPage, float lightDepth)
{
	vec2 uv = shadowPosition.xy * shadowMapSize;

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSize.x, 1.0 / shadowMapSize.y );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float gradientNoise = TWO_PI * InterleavedGradientNoise(gl_FragCoord.xy);

	float penumbra = Penumbra(shadowTexture, gradientNoise, base_uv, lightDepth, int(NUM_SOFT_SHADOW_SAMPLES), shadowMapSizeInv, shadowMapPage);

	float shadow = 0.0;
	for(int i = 0; i < NUM_SOFT_SHADOW_SAMPLES; ++i)
	{
		vec2 sampleUV = VogelDiskSample(i, int(NUM_SOFT_SHADOW_SAMPLES), gradientNoise);
		sampleUV = base_uv + sampleUV * penumbra * SHADOW_FILTER_MAX_SIZE;

		shadow += SampleShadowMap(shadowTexture, sampleUV, 0, 0, shadowMapSizeInv, shadowMapPage, lightDepth);
	}

	return shadow / NUM_SOFT_SHADOW_SAMPLES;
}

float OptimizedPCF7(Tex2DAddress shadowTexture, vec3 shadowPosition, vec2 shadowMapSize, float shadowMapPage, float lightDepth)
{
	vec2 uv = shadowPosition.xy * shadowMapSize;

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSize.x, 1.0 / shadowMapSize.y );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0;

	float uw0 = (5 * s - 6);
    float uw1 = (11 * s - 28);
    float uw2 = -(11 * s + 17);
    float uw3 = -(5 * s + 1);

    float u0 = (4 * s - 5) / uw0 - 3;
    float u1 = (4 * s - 16) / uw1 - 1;
    float u2 = -(7 * s + 5) / uw2 + 1;
    float u3 = -s / uw3 + 3;

    float vw0 = (5 * t - 6);
    float vw1 = (11 * t - 28);
    float vw2 = -(11 * t + 17);
    float vw3 = -(5 * t + 1);

    float v0 = (4 * t - 5) / vw0 - 3;
    float v1 = (4 * t - 16) / vw1 - 1;
    float v2 = -(7 * t + 5) / vw2 + 1;
    float v3 = -t / vw3 + 3;

    sum += uw0 * vw0 * SampleShadowMap(shadowTexture, base_uv, u0, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw1 * vw0 * SampleShadowMap(shadowTexture, base_uv, u1, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw2 * vw0 * SampleShadowMap(shadowTexture, base_uv, u2, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw3 * vw0 * SampleShadowMap(shadowTexture, base_uv, u3, v0, shadowMapSizeInv, shadowMapPage, lightDepth);

    sum += uw0 * vw1 * SampleShadowMap(shadowTexture, base_uv, u0, v1, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw1 * vw1 * SampleShadowMap(shadowTexture, base_uv, u1, v1, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw2 * vw1 * SampleShadowMap(shadowTexture, base_uv, u2, v1, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw3 * vw1 * SampleShadowMap(shadowTexture, base_uv, u3, v1, shadowMapSizeInv, shadowMapPage, lightDepth);

    sum += uw0 * vw2 * SampleShadowMap(shadowTexture, base_uv, u0, v2, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw1 * vw2 * SampleShadowMap(shadowTexture, base_uv, u1, v2, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw2 * vw2 * SampleShadowMap(shadowTexture, base_uv, u2, v2, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw3 * vw2 * SampleShadowMap(shadowTexture, base_uv, u3, v2, shadowMapSizeInv, shadowMapPage, lightDepth);

    sum += uw0 * vw3 * SampleShadowMap(shadowTexture, base_uv, u0, v3, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw1 * vw3 * SampleShadowMap(shadowTexture, base_uv, u1, v3, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw2 * vw3 * SampleShadowMap(shadowTexture, base_uv, u2, v3, shadowMapSizeInv, shadowMapPage, lightDepth);
    sum += uw3 * vw3 * SampleShadowMap(shadowTexture, base_uv, u3, v3, shadowMapSizeInv, shadowMapPage, lightDepth);

    return sum * (1.0f / 2704.0f);
}

float OptimizedPCF5(Tex2DAddress shadowTexture, vec3 shadowPosition, vec2 shadowMapSize, float shadowMapPage, float lightDepth)
{
	vec2 uv = shadowPosition.xy * shadowMapSize;

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSize.x, 1.0 / shadowMapSize.y );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0;

	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);

	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;

	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);

	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;

	sum += uw0 * vw0 *  SampleShadowMap(shadowTexture, base_uv, u0, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw1 * vw0 *  SampleShadowMap(shadowTexture, base_uv, u1, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw2 * vw0 *  SampleShadowMap(shadowTexture, base_uv, u2, v0, shadowMapSizeInv, shadowMapPage, lightDepth);

	sum += uw0 * vw1 *  SampleShadowMap(shadowTexture, base_uv, u0, v1, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw1 * vw1 *  SampleShadowMap(shadowTexture, base_uv, u1, v1, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw2 * vw1 *  SampleShadowMap(shadowTexture, base_uv, u2, v1, shadowMapSizeInv, shadowMapPage, lightDepth);

	sum += uw0 * vw2 *  SampleShadowMap(shadowTexture, base_uv, u0, v2, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw1 * vw2 *  SampleShadowMap(shadowTexture, base_uv, u1, v2, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw2 * vw2 *  SampleShadowMap(shadowTexture, base_uv, u2, v2, shadowMapSizeInv, shadowMapPage, lightDepth);

	return sum * 0.006944444f; //1 / 144
}

float OptimizedPCF3(Tex2DAddress shadowTexture, vec3 shadowPosition, vec2 shadowMapSize, float shadowMapPage, float lightDepth)
{
	//Using FilterSize_ == 3 from https://github.com/TheRealMJP/Shadows/blob/master/Shadows/Mesh.hlsl - SampleShadowMapOptimizedPCF - from the witness
	vec2 uv = shadowPosition.xy * shadowMapSize;

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSize.x, 1.0 / shadowMapSize.y );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0;

	float uw0 = (3 - 2 * s);
	float uw1 = (1 + 2 * s);

	float u0 = (2 - s) / uw0 - 1;
	float u1 = s / uw1 + 1;

	float vw0 = (3 - 2 * t);
	float vw1 = (1 + 2 * t);

	float v0 = (2 - t) / vw0 - 1;
	float v1 = t / vw1 + 1;

	sum += uw0 * vw0 * SampleShadowMap(shadowTexture, base_uv, u0, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw1 * vw0 * SampleShadowMap(shadowTexture, base_uv, u1, v0, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw0 * vw1 * SampleShadowMap(shadowTexture, base_uv, u0, v1, shadowMapSizeInv, shadowMapPage, lightDepth);
	sum += uw1 * vw1 * SampleShadowMap(shadowTexture, base_uv, u0, v1, shadowMapSizeInv, shadowMapPage, lightDepth);

	return sum * 0.0625; //1 / 16
}

#endif