#ifndef GLSL_BLOOM_PARAMS
#define GLSL_BLOOM_PARAMS

layout (std140, binding = 5) uniform BloomParams
{
	vec4 bloomFilter; //x - threshold, y = threshold - knee, z = 2.0f * knee, w = 0.25f / knee 
	uvec4 bloomResolutions;
	vec4 bloomFilterRadiusIntensity;
	//@TODO(Serge): clean this up - don't want to use uint64_t if we don't have to
#ifdef GL_NV_gpu_shader5
	uint64_t textureContainerToSample;
#else
	sampler2DArray textureContainerToSample;
#endif
	float texturePageToSample;
	float textureLodToSample;	
};

#endif