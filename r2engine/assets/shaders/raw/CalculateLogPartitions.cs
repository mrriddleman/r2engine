#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;


struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct Partition
{
	vec4 intervalBeginScale;
	vec4 intervalEndBias;
};

struct UPartition
{
	uvec4 intervalBeginMinCoord;
	uvec4 intervalEndMaxCoord;
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

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions[NUM_FRUSTUM_SPLITS];
	UPartition gPartitionsU[NUM_FRUSTUM_SPLITS];
};

float LogPartitionFromRange(uint part, float minZ, float maxZ);

void main(void)
{
	float minZ = uintBitsToFloat(gPartitionsU[0].intervalBeginMinCoord.x);
	float maxZ = uintBitsToFloat(gPartitionsU[NUM_FRUSTUM_SPLITS - 1].intervalEndMaxCoord.x);

	uint cascadeIndex = gl_LocalInvocationIndex;

	gPartitions[cascadeIndex].intervalBeginScale.x = LogPartitionFromRange(cascadeIndex, minZ, maxZ);
	gPartitions[cascadeIndex].intervalEndBias.x = LogPartitionFromRange(cascadeIndex + 1, minZ, maxZ);

	//gPartitionsU[cascadeIndex].intervalBegin = floatBitsToUint(gPartitions[cascadeIndex].intervalBegin);
	//gPartitionsU[cascadeIndex].intervalEnd = floatBitsToUint(gPartitions[cascadeIndex].intervalEnd);
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

		float lambda = 1;
		float ratio = maxZ / minZ;
		float power = float(part + 1) * (1.0 / float(NUM_FRUSTUM_SPLITS));
		float logScale = minZ * pow(ratio, power);
		float uniformScale = minZ + range * power;
		float d = (logScale - uniformScale) + uniformScale;
		z = lambda * (d - nearClip) / clipRange;

	}

	return z;
}