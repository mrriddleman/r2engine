#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define WARP_SIZE 32

layout (local_size_x = WARP_SIZE, local_size_y = WARP_SIZE, local_size_z = 1) in;

//layout (binding = 0, r11f_g11f_b10f) uniform image2D inputImage;
layout (binding = 0, r11f_g11f_b10f) uniform image2D inputImage2;
layout (binding = 1, r11f_g11f_b10f) uniform image2D outputImage;

layout (std140, binding = 5) uniform BloomParams
{
	vec4 bloomFilter; //x - threshold, y = threshold - knee, z = 2.0f * knee, w = 0.25f / knee 
	uvec4 bloomResolutions;
	vec4 bloomFilterRadius;

	uint64_t textureContainerToSample;
	float texturePageToSample;
	float textureLodToSample;
};

void main()
{
	ivec2 outTexCoord = ivec2( gl_GlobalInvocationID.xy );
	outTexCoord = clamp(outTexCoord, ivec2(0), ivec2(bloomResolutions.z, bloomResolutions.w));

	vec2 texCoordf = vec2(outTexCoord) / vec2(bloomResolutions.z, bloomResolutions.w);
	
	float x = 1.0f / float(bloomResolutions.x);
	float y = 1.0f / float(bloomResolutions.y);

	vec3 a = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - x, texCoordf.y + y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x - x, texCoord.y + y)).rgb;
	vec3 b = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x    , texCoordf.y + y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y + y)).rgb;
	vec3 c = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + x, texCoordf.y + y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x + x, texCoord.y + y)).rgb;

	vec3 d = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - x, texCoordf.y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x - x, texCoord.y)).rgb;
	vec3 e = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x    , texCoordf.y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y)).rgb;
	vec3 f = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + x, texCoordf.y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x + x, texCoord.y)).rgb;

	vec3 g = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x - x, texCoordf.y - y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x - x, texCoord.y - y)).rgb;
	vec3 h = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x    , texCoordf.y - y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y - y)).rgb;
	vec3 i = textureLod(sampler2DArray(textureContainerToSample), vec3(texCoordf.x + x, texCoordf.y - y, texturePageToSample), textureLodToSample).rgb;//imageLoad(inputImage, ivec2(texCoord.x + x, texCoord.y - y)).rgb;

	vec3 upsample = e * 4.0;
	upsample += (b+d+f+h) * 2.0;
	upsample += (a+c+g+i);
	upsample *= 1.0 / 16.0;

	vec3 curImageColor = imageLoad(inputImage2, outTexCoord).rgb;

	imageStore(outputImage, outTexCoord, vec4(upsample + curImageColor, 1));
}

