#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
const uint MAX_NUM_LIGHTS = 50;
#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;


struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};



struct BoundsUint
{
    uvec4 minCoord;
    uvec4 maxCoord;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspect;
};


//@NOTE(Serge): this is in the order of the render target surfaces in RenderTarget.h
layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
};

struct Partition
{
	vec4 intervalBegin;
	vec4 intervalEnd;
};

struct UPartition
{
	uvec4 intervalBegin;
	uvec4 intervalEnd;
};

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions;
	UPartition gPartitionsU;

	vec4 gScale[NUM_FRUSTUM_SPLITS][MAX_NUM_LIGHTS];
	vec4 gBias[NUM_FRUSTUM_SPLITS][MAX_NUM_LIGHTS];

	mat4 gShadowMatrix[MAX_NUM_LIGHTS];

	float gSpotLightShadowMapPages[NUM_SPOTLIGHT_SHADOW_PAGES];
	float gPointLightShadowMapPages[NUM_POINTLIGHT_SHADOW_PAGES];
	float gDirectionLightShadowMapPages[NUM_DIRECTIONLIGHT_SHADOW_PAGES];
};


float LogPartitionFromRange(uint part, float minZ, float maxZ);

void main(void)
{
	float minZ = uintBitsToFloat(gPartitionsU.intervalBegin[0]);
	float maxZ = uintBitsToFloat(gPartitionsU.intervalEnd[NUM_FRUSTUM_SPLITS-1]);

	uint cascadeIndex = gl_LocalInvocationIndex;

	gPartitions.intervalBegin[cascadeIndex] = LogPartitionFromRange(cascadeIndex, minZ, maxZ);
	gPartitions.intervalEnd[cascadeIndex] = LogPartitionFromRange(cascadeIndex + 1, minZ, maxZ);

//	gPartitionsU[cascadeIndex].intervalBeginMinCoord.x = floatBitsToUint(gPartitions[cascadeIndex].intervalBeginScale.x);
//	gPartitionsU[cascadeIndex].intervalEndMaxCoord.x = floatBitsToUint(gPartitions[cascadeIndex].intervalEndBias.x);
}


float LogPartitionFromRange(uint part, float minDistance, float maxDistance)
{
	//From: https://github.com/TheRealMJP/Shadows/blob/master/Shadows/SetupShadows.hlsl

	float z = maxDistance;

	if(part < NUM_FRUSTUM_SPLITS)
	{
		float nearClip = exposureNearFar.y;
		float farClip = exposureNearFar.z;
		float clipRange = farClip - nearClip;

		float minZ = nearClip + minDistance * clipRange;
		float maxZ = nearClip + maxDistance * clipRange;

		float range = maxZ - minZ;

		float lambda = 1.2;
		float ratio = maxZ / minZ;
		float power = float(part + 1) * (1.0 / float(NUM_FRUSTUM_SPLITS));
		float logScale = minZ * pow(abs(ratio), power);
		float uniformScale = minZ + range * power;
		float d = lambda * (logScale - uniformScale) + uniformScale;
		z =  (d - nearClip) / clipRange;

	}

	return z;
}