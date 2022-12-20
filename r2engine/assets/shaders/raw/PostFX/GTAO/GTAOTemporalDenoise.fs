//Taken from: https://github.com/krzysztofmarecki/OpenGL/blob/master/OpenGL/src/shaders/gtaoTemporalDenoiser.frag

#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define RATE_OF_CHANGE 0.5

#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Depth/DepthUtils.glsl"

layout (location = 0) out float FragColor;

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

float SampleTextureF(Tex2DAddress tex, vec2 uv, vec2 offset)
{
	vec3 texCoord = vec3(uv + offset, tex.page);
	return texture(sampler2DArray(tex.container), texCoord).r;
}

vec4 GatherOffset(Tex2DAddress tex, vec2 uv, ivec2 offset)
{
	vec3 texCoord = vec3(uv, tex.page);
	return textureGatherOffset(sampler2DArray(tex.container), texCoord, offset);
}

float VsDepthFromCsDepth(float clipSpaceDepth, float near) {
	return -near / clipSpaceDepth;
}

void main()
{
	const float ao = SampleTextureF(ambientOcclusionDenoiseSurface, fs_in.texCoords.xy, vec2(0));
	const vec2 velocity = CalculateVelocity(zPrePassShadowsSurface[0], fs_in.texCoords.xy);

	const vec2 uvMinusVel = fs_in.texCoords.xy - velocity;

	vec4 depthPrev4 = GatherOffset(zPrePassShadowsSurface[1], uvMinusVel, ivec2(0));

	vec4 aoAcc4 = vec4(SampleTextureF(ambientOcclusionTemporalDenoiseSurface[1], uvMinusVel, ivec2(0)));

	const vec2 pixelUVPrev = uvMinusVel * fovAspectResXResY.zw- vec2(0.5) + vec2(1.0 / 512);
	const float weightX = 1 - fract(pixelUVPrev.x);
	const float weightY = fract(pixelUVPrev.y);

	float bilinearWeights[4];

	bilinearWeights[0] = weightX * weightY;
	bilinearWeights[1] = (1.0 - weightX) * weightY;
	bilinearWeights[2] = (1.0 - weightX) * (1.0 - weightY);
	bilinearWeights[3] = weightX * (1.0 - weightY);

	const float depthCurr = SampleTextureF(zPrePassShadowsSurface[0], fs_in.texCoords.xy, vec2(0));
	const float vsDepthCurr = VsDepthFromCsDepth(depthCurr, exposureNearFar.y);

	float weight = 0;
	float aoAccWeighted = 0;

	for(int i = 0; i < 4; ++i)
	{
		const float vsDepthPrev = VsDepthFromCsDepth(depthPrev4[i], exposureNearFar.y);

		const float bilateralWeight = clamp(1.0 + 0.1 * (vsDepthCurr - vsDepthPrev), 0.01, 1.0);
		weight += bilinearWeights[i] * bilateralWeight;
		aoAccWeighted += bilinearWeights[i] * bilateralWeight * aoAcc4[i];
	}

	aoAccWeighted /= weight;

	float rateOfChange = RATE_OF_CHANGE;

	const vec2 uvPrevDistanceToMiddle = abs(uvMinusVel - vec2(0.5));
	if(uvPrevDistanceToMiddle.x > 0.5 || uvPrevDistanceToMiddle.y > 0.5)
		rateOfChange = 1;

	float vsDepthPrev;
	if(fract(pixelUVPrev.x) > 0.5)
	{
		if(fract(pixelUVPrev.y) > 0.5)
			vsDepthPrev = depthPrev4.y;
		else
			vsDepthPrev = depthPrev4.z;
	}
	else
	{
		if(fract(pixelUVPrev.y) > 0.5)
			vsDepthPrev = depthPrev4.x;
		else
			vsDepthPrev = depthPrev4.w;
	}

	vsDepthPrev = VsDepthFromCsDepth(vsDepthPrev, exposureNearFar.y);

	if((-vsDepthCurr * 0.9) > -vsDepthPrev)
		rateOfChange = 1;


	FragColor = mix(aoAccWeighted, ao, rateOfChange);
}
