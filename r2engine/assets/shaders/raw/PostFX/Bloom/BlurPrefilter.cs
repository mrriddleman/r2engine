#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define WARP_SIZE 32

layout (local_size_x = WARP_SIZE, local_size_y = WARP_SIZE, local_size_z = 1) in;

layout (binding = 0, r11f_g11f_b10f) uniform image2D outputImage;

#include "Input/UniformBuffers/BloomParams.glsl"

void main()
{
	ivec2 outTexCoord = ivec2( gl_GlobalInvocationID.xy);

	outTexCoord = clamp(outTexCoord, ivec2(0), ivec2(bloomResolutions.z, bloomResolutions.w));

	vec2 textureCoord = vec2(outTexCoord) / vec2(bloomResolutions.z, bloomResolutions.w);

	vec3 result = vec3(0);

	//@NOTE(Serge): currently we're using this to offset the blur to shift it back to the left. Not sure why it's pulling right all the time
	for(int x = -2; x < 6; ++x)
	{
	 	for (int y = -2; y < 6; ++y) 
        {
         	result += textureLodOffset(sampler2DArray(textureContainerToSample), vec3(textureCoord, texturePageToSample), textureLodToSample, ivec2(x,y) ).rgb;
        }
	}

	result /= 64.0f;

	imageStore(outputImage, outTexCoord, vec4(result, 1));
}