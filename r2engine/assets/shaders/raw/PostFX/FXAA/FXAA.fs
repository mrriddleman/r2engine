#version 450 core

//https://github.com/McNopper/OpenGL/blob/master/Example42/shader/fxaa.frag.glsl

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/UniformBuffers/AAParams.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


void main()
{
	vec3 rgbM = SampleTexture(compositeSurface, fs_in.texCoords.xy, ivec2(0));

	vec3 rgbNW = SampleTexture(compositeSurface, fs_in.texCoords.xy, ivec2(-1, 1));
	vec3 rgbNE = SampleTexture(compositeSurface, fs_in.texCoords.xy, ivec2( 1, 1));
	vec3 rgbSW = SampleTexture(compositeSurface, fs_in.texCoords.xy, ivec2(-1, -1));
	vec3 rgbSE = SampleTexture(compositeSurface, fs_in.texCoords.xy, ivec2(1, -1));

	// see http://en.wikipedia.org/wiki/Grayscale
	const vec3 toLuma = vec3(0.299, 0.587, 0.114);

	//convert from RGB to luma
	float lumaNW = dot(rgbNW, toLuma);
	float lumaNE = dot(rgbNE, toLuma);
	float lumaSW = dot(rgbNW, toLuma);
	float lumaSE = dot(rgbSE, toLuma);
	float lumaM  = dot(rgbM, toLuma);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = min(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	if(lumaMax - lumaMin <= lumaMax * fxaa_lumaThreshold)
	{
		FragColor = vec4(rgbM, 1.0);
		return;
	}

	vec2 samplingDirection;
	samplingDirection.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	samplingDirection.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float sampleingDirectionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * fxaa_lumaMulReduce, fxaa_lumaMinReduce);

	float minSamplingDirectionFactor = 1.0 / (min(abs(samplingDirection.x), abs(samplingDirection.y)) + sampleingDirectionReduce);

	samplingDirection = clamp(samplingDirection * minSamplingDirectionFactor, vec2(-fxaa_maxSpan), vec2(fxaa_maxSpan)) * fxaa_texelStep;

	vec3 rgbSampleNeg = SampleTexture(compositeSurface, fs_in.texCoords.xy + samplingDirection * (1.0/3.0 - 0.5), ivec2(0));
	vec3 rgbSamplePos = SampleTexture(compositeSurface, fs_in.texCoords.xy + samplingDirection * (2.0/3.0 - 0.5), ivec2(0));

	vec3 rgbTwoTab = (rgbSamplePos + rgbSampleNeg) * 0.5;

	vec3 rgbSampleNegOuter = SampleTexture(compositeSurface, fs_in.texCoords.xy + samplingDirection * (0.0/3.0 - 0.5), ivec2(0));
	vec3 rgbSamplePosOuter = SampleTexture(compositeSurface, fs_in.texCoords.xy + samplingDirection * (3.0/3.0 - 0.5), ivec2(0));

	vec3 rgbFourTab = (rgbSamplePosOuter + rgbSampleNegOuter) * 0.25 + rgbTwoTab * 0.5;

	float lumaFourTab = dot(rgbFourTab, toLuma);

	FragColor = vec4(rgbFourTab, 1.0);
	
	if (lumaFourTab < lumaMin || lumaFourTab > lumaMax)
	{
		// ... yes, so use only two samples.
		FragColor = vec4(rgbTwoTab, 1.0); 
	}
}
