#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
#define INV_SQRT_OF_2PI 0.39894228040143267793994605993439  // 1.0/SQRT_OF_2PI
#define INV_PI          0.31830988618379067153776752674503

layout (location = 0) out vec2 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspectResXResY;
    uint64_t frame;
    uint64_t unused;
    uvec4 tileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
    vec4 jitter; // {currJitterX, currJitterY, prevJitterX, prevJitterY}
};

layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
	Tex2DAddress ambientOcclusionDenoiseSurface;
	Tex2DAddress zPrePassShadowsSurface[2];
	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
	Tex2DAddress normalSurface;
	Tex2DAddress specularSurface;
	Tex2DAddress ssrSurface;
	Tex2DAddress convolvedGBUfferSurface[2];
	Tex2DAddress ssrConeTracedSurface;
	Tex2DAddress bloomDownSampledSurface;
	Tex2DAddress bloomBlurSurface;
	Tex2DAddress bloomUpSampledSurface;
};


in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


vec2 SampleTexture(Tex2DAddress inputTexture, ivec2 uv, ivec2 uvOffset)
{
	ivec3 texCoord = ivec3(uv.x + uvOffset.x, uv.y + uvOffset.y, inputTexture.page);
	return texelFetch(sampler2DArray(inputTexture.container), texCoord, 0).rg;
}

vec2 SampleTextureF(Tex2DAddress tex, vec2 uv, vec2 offset)
{
	vec3 texCoord = vec3(uv + offset, tex.page);
	return texture(sampler2DArray(tex.container), texCoord).rg;
}

vec4 GatherOffset(Tex2DAddress tex, vec2 uv, ivec2 offset)
{
	vec3 texCoord = vec3(uv, tex.page);
	return textureGatherOffset(sampler2DArray(tex.container), texCoord, offset);
}

vec2 BasicBlur()
{
	//@TODO(Serge): This is kinda bad since we're hard coding the ao texture - would be nice to not have to do that
	//				Instead - we would need to pass a Tex2DAddress uniform 
	vec2 center = SampleTexture(ambientOcclusionSurface, ivec2(gl_FragCoord.xy), ivec2(0));
	float rampMaxInv = 1.0 / (center.y * 0.1);

	float totalAO = 0.0;
	float totalWeight = 0.0;

	for(int i = -2; i < 2 ; ++i)
	{
		for(int j = -2; j < 2; ++j)
		{
			//@TODO(Serge): This is kinda back since we're hard coding the ao texture - would be nice to not have to do that
			//				Instead - we would need to pass a Tex2DAddress uniform 
			vec2 S = SampleTexture(ambientOcclusionSurface, ivec2(gl_FragCoord.xy), ivec2(i, j));

			float weight = clamp(1.0 - (abs(S.y - center.y) * rampMaxInv), 0.0, 1.0);
			totalAO += S.x * weight;
			totalWeight += weight;
		}
	}

	float ao = totalAO / totalWeight;

	return vec2(ao, center.y);
}

float VsDepthFromCsDepth(float clipSpaceDepth, float near) {
	return -near / clipSpaceDepth;
}


vec2 SpatialDenoise()
{
	vec4[4] ao4s;
	vec4[4] depth4s;

	ao4s[0] = GatherOffset(ambientOcclusionSurface, fs_in.texCoords.xy, ivec2(0));
	ao4s[1] = GatherOffset(ambientOcclusionSurface, fs_in.texCoords.xy, ivec2(0, -2));
	ao4s[2] = GatherOffset(ambientOcclusionSurface, fs_in.texCoords.xy, ivec2(-2, 0));
	ao4s[3] = GatherOffset(ambientOcclusionSurface, fs_in.texCoords.xy, ivec2(-2, -2));

	depth4s[0] = GatherOffset(zPrePassShadowsSurface[0], fs_in.texCoords.xy, ivec2(0));
	depth4s[1] = GatherOffset(zPrePassShadowsSurface[0], fs_in.texCoords.xy, ivec2(0, -2));
	depth4s[2] = GatherOffset(zPrePassShadowsSurface[0], fs_in.texCoords.xy, ivec2(-2, 0));
	depth4s[3] = GatherOffset(zPrePassShadowsSurface[0], fs_in.texCoords.xy, ivec2(-2, -2));


	const float near = exposureNearFar.y;
	const float depthCurrent = VsDepthFromCsDepth(depth4s[0].w, near);

	float ao = 0;
	float weight = 0;
	const float threshold = abs(near * depthCurrent);

	for(int i = 0; i < 4; ++i)
	{
		for(int j = 0; j < 4; j++)
		{
			const float depthDiff = abs(VsDepthFromCsDepth(depth4s[i][j], near) - depthCurrent);
			if(depthDiff < threshold)
			{
				const float weightCurrent = 1.0 - clamp(10.0 * depthDiff / threshold, 0.0, 1.0);
				ao += ao4s[i][j] * weightCurrent;
				weight += weightCurrent;
			}
		}
	}


	return vec2(ao/ weight, 0);	
}


void main(void)
{
//	FragColor = BasicBlur();

	FragColor = SpatialDenoise();
}