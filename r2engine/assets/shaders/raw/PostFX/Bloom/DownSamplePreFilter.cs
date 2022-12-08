#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define WARP_SIZE 32

layout (local_size_x = WARP_SIZE, local_size_y = WARP_SIZE, local_size_z = 1) in;

layout (binding = 0, r11f_g11f_b10f) uniform image2D outputImage;

#include "Input/UniformBuffers/BloomParams.glsl"

float RGBToLuminance(vec3 rgb)
{
	return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}

vec3 PreFilter(vec3 c)
{
	float brightness = max(c.r, max(c.g, c.b));
	float soft = brightness - bloomFilter.y;
	soft = clamp(soft, 0, bloomFilter.z);
	soft = soft * soft * bloomFilter.w;
	float contribution = max(soft, brightness - bloomFilter.x);
	contribution /= max(brightness, 0.0001);
	return c * contribution;
}

void main()
{
	//gl_GlobalInvocationID = gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID
	
	ivec2 outTexCoord = ivec2( gl_GlobalInvocationID.xy);
	outTexCoord = clamp(outTexCoord, ivec2(0), ivec2(bloomResolutions.z, bloomResolutions.w));

	vec2 texCoordf = (vec2(outTexCoord)) / vec2(bloomResolutions.z, bloomResolutions.w);

	float x = bloomFilterRadiusIntensity.x;
	float y = bloomFilterRadiusIntensity.y;

	vec3 a = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - 2 * x, texCoordf.y + 2 * y, texturePageToSample), textureLodToSample).rgb; 
	vec3 b = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x        , texCoordf.y + 2 * y, texturePageToSample), textureLodToSample).rgb;
	vec3 c = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + 2 * x, texCoordf.y + 2 * y, texturePageToSample), textureLodToSample).rgb; 

	vec3 d = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - 2 * x, texCoordf.y, texturePageToSample), textureLodToSample).rgb;
	vec3 e = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x        , texCoordf.y, texturePageToSample), textureLodToSample).rgb;
	vec3 f = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + 2 * x, texCoordf.y, texturePageToSample), textureLodToSample).rgb;

	vec3 g = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - 2 * x, texCoordf.y - 2 * y, texturePageToSample), textureLodToSample).rgb;
	vec3 h = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x        , texCoordf.y - 2 * y, texturePageToSample), textureLodToSample).rgb;
	vec3 i = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + 2 * x, texCoordf.y - 2 * y, texturePageToSample), textureLodToSample).rgb;

	vec3 j = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - x, texCoordf.y + y, texturePageToSample), textureLodToSample).rgb;
	vec3 k = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + x, texCoordf.y + y, texturePageToSample), textureLodToSample).rgb;
	vec3 l = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - x, texCoordf.y - y, texturePageToSample), textureLodToSample).rgb;
	vec3 m = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + x, texCoordf.y - y, texturePageToSample), textureLodToSample).rgb;

	vec3 downSample = e * 0.125;

	downSample += (a+c+g+i) * 0.03125;
	downSample += (b+d+f+h) * 0.0625;
	downSample += (j+k+l+m) * 0.125;

	downSample = PreFilter(downSample);

	downSample = max(downSample, 0.0001f);

	imageStore(outputImage, outTexCoord, vec4(downSample, 0));
}

