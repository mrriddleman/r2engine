#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define WARP_SIZE 32

layout (local_size_x = WARP_SIZE, local_size_y = WARP_SIZE, local_size_z = 1) in;

layout (binding = 0, r11f_g11f_b10f) uniform image2D inputImage;
layout (binding = 1, r11f_g11f_b10f) uniform image2D inputImage2;
layout (binding = 2, r11f_g11f_b10f) uniform image2D outputImage;

layout (std140, binding = 5) uniform BloomParams
{
	vec4 bloomFilter; //x - threshold, y = threshold - knee, z = 2.0f * knee, w = 0.25f / knee 
	uvec4 bloomResolutions;
	vec4 bloomFilterRadius;
};

void main()
{
	float x = bloomFilterRadius.x;
	float y = bloomFilterRadius.y;

	ivec2 outTexCoord = ivec2( gl_GlobalInvocationID.xy );
	ivec2 texCoord = ivec2(vec2(outTexCoord) * 0.5f);

	outTexCoord = clamp(outTexCoord, ivec2(0), ivec2(bloomResolutions.z, bloomResolutions.w));
	texCoord = clamp(texCoord, ivec2(0), ivec2(bloomResolutions.x, bloomResolutions.y));
	


	vec3 a = imageLoad(inputImage, ivec2(texCoord.x - x, texCoord.y + y)).rgb;
	vec3 b = imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y + y)).rgb;
	vec3 c = imageLoad(inputImage, ivec2(texCoord.x + x, texCoord.y + y)).rgb;

	vec3 d = imageLoad(inputImage, ivec2(texCoord.x - x, texCoord.y)).rgb;
	vec3 e = imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y)).rgb;
	vec3 f = imageLoad(inputImage, ivec2(texCoord.x + x, texCoord.y)).rgb;

	vec3 g = imageLoad(inputImage, ivec2(texCoord.x - x, texCoord.y - y)).rgb;
	vec3 h = imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y - y)).rgb;
	vec3 i = imageLoad(inputImage, ivec2(texCoord.x + x, texCoord.y - y)).rgb;

	vec3 upsample = e * 4.0;
	upsample += (b+d+f+h) * 2.0;
	upsample += (a+c+g+i);
	upsample *= 1.0 / 16.0;

	vec3 curImageColor = imageLoad(inputImage2, outTexCoord).rgb;

	imageStore(outputImage, outTexCoord, vec4(curImageColor + upsample, 1));

}

