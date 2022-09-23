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
	//gl_GlobalInvocationID = gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID
	
	ivec2 outTexCoord = ivec2( gl_GlobalInvocationID.xy);

	vec2 texCoordf = (vec2(outTexCoord)) / vec2(bloomResolutions.z, bloomResolutions.w);

	ivec2 texCoord = ivec2(round(texCoordf.x  * float(bloomResolutions.x)), round(texCoordf.y  * float(bloomResolutions.y)) );

	outTexCoord = clamp(outTexCoord, ivec2(0), ivec2(bloomResolutions.z, bloomResolutions.w));
	texCoord = clamp(texCoord, ivec2(0), ivec2(bloomResolutions.x, bloomResolutions.y));

	vec3 a = imageLoad(inputImage, ivec2(texCoord.x - 2, texCoord.y + 2)).rgb;
	vec3 b = imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y + 2)).rgb;
	vec3 c = imageLoad(inputImage, ivec2(texCoord.x + 2, texCoord.y + 2)).rgb;

	vec3 d = imageLoad(inputImage, ivec2(texCoord.x - 2, texCoord.y)).rgb;
	vec3 e = imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y)).rgb;
	vec3 f = imageLoad(inputImage, ivec2(texCoord.x + 2, texCoord.y)).rgb;

	vec3 g = imageLoad(inputImage, ivec2(texCoord.x - 2, texCoord.y - 2)).rgb;
	vec3 h = imageLoad(inputImage, ivec2(texCoord.x    , texCoord.y - 2)).rgb;
	vec3 i = imageLoad(inputImage, ivec2(texCoord.x + 2, texCoord.y - 2)).rgb;

	vec3 j = imageLoad(inputImage, ivec2(texCoord.x - 1, texCoord.y + 1)).rgb;
	vec3 k = imageLoad(inputImage, ivec2(texCoord.x + 1, texCoord.y + 1)).rgb;
	vec3 l = imageLoad(inputImage, ivec2(texCoord.x - 1, texCoord.y - 1)).rgb;
	vec3 m = imageLoad(inputImage, ivec2(texCoord.x + 1, texCoord.y - 1)).rgb;

	vec3 downSample =  e * 0.125;

	downSample += (a+c+g+i) * 0.03125;
	downSample += (b+d+f+h) * 0.0625;
	downSample += (j+k+l+m) * 0.125;

	downSample = PreFilter(downSample);

	imageStore(outputImage, outTexCoord, vec4(downSample, 0));
}

