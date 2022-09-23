#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define WARP_SIZE 32

layout (local_size_x = WARP_SIZE, local_size_y = WARP_SIZE, local_size_z = 1) in;


layout (binding = 0, r11f_g11f_b10f) uniform image2D inputImage;
layout (binding = 1, r11f_g11f_b10f) uniform image2D outputImage;


layout (std140, binding = 5) uniform BloomParams
{
	vec4 bloomFilter; //x - threshold, y = threshold - knee, z = 2.0f * knee, w = 0.25f / knee 
	uvec4 bloomResolutions;
	vec4 bloomFilterRadiusIntensity;
};

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
	ivec2 outTexCoord = ivec2( gl_GlobalInvocationID.xy);

	outTexCoord = clamp(outTexCoord, ivec2(0), ivec2(bloomResolutions.z, bloomResolutions.w));

	vec3 result = vec3(0);

	for(int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y) 
        {
        	result += imageLoad(inputImage, ivec2(outTexCoord.x + x, outTexCoord.y + y)).rgb;
        }
	}

	result /= 16.0f;

	imageStore(outputImage, outTexCoord, vec4(result, 1));
}