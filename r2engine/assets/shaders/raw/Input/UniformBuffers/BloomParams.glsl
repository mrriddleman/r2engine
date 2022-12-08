#ifndef GLSL_BLOOM_PARAMS
#define GLSL_BLOOM_PARAMS

layout (std140, binding = 5) uniform BloomParams
{
	vec4 bloomFilter; //x - threshold, y = threshold - knee, z = 2.0f * knee, w = 0.25f / knee 
	uvec4 bloomResolutions;
	vec4 bloomFilterRadiusIntensity;

	uint64_t textureContainerToSample;
	float texturePageToSample;
	float textureLodToSample;	
};

#endif