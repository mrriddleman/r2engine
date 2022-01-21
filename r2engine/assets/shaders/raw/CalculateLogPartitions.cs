#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1) in;


struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct Partition
{
	float intervalBegin;
	float intervalEnd;

	vec3 scale;
	vec3 bias;
};

struct UPartition
{
	uint intervalBegin;
	uint intervalEnd;

	vec3 scale;
	vec3 bias;
};

struct BoundsUint
{
    uvec3 minCoord;
    uvec3 maxCoord;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
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
	BoundsUint gPartitionBoundsU[NUM_FRUSTUM_SPLITS];
};

float LogPartitionFromRange(uint part, float minZ, float maxZ);

void main(void)
{
	float minZ = uintBitsToFloat(gPartitionsU[0].intervalBegin);
	float maxZ = uintBitsToFloat(gPartitionsU[NUM_FRUSTUM_SPLITS - 1].intervalEnd);

	uint cascadeIndex = gl_LocalInvocationIndex;

	gPartitions[cascadeIndex].intervalBegin = cascadeIndex == 0 ? exposureNearFar.y : LogPartitionFromRange(cascadeIndex, minZ, maxZ);
	gPartitions[cascadeIndex].intervalEnd = cascadeIndex == (NUM_FRUSTUM_SPLITS - 1) ? exposureNearFar.z : LogPartitionFromRange(cascadeIndex + 1, minZ, maxZ);

	//gPartitionsU[cascadeIndex].intervalBegin = floatBitsToUint(gPartitions[cascadeIndex].intervalBegin);
	//gPartitionsU[cascadeIndex].intervalEnd = floatBitsToUint(gPartitions[cascadeIndex].intervalEnd);
}


float LogPartitionFromRange(uint part, float minZ, float maxZ)
{
	float z = maxZ;

	if(part < NUM_FRUSTUM_SPLITS)
	{
		float ratio = maxZ / minZ;
		float power = float(part) * (1.0 / float(NUM_FRUSTUM_SPLITS));
		z = minZ * pow(ratio, power);
	}

	return z;
}