#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;

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

struct BoundsFloat
{
    vec3 minCoord;
    vec3 maxCoord;
};

struct BoundsUint
{
    uvec3 minCoord;
    uvec3 maxCoord;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposure;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
};

layout (std140, binding = 3) uniform SDSMParams
{
	vec4 lightSpaceBorder;
	vec4 maxScale;
	float dilationFactor;
	uint scatterTileDim;
	uint reduceTileDim;
	uint padding;
};

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions[NUM_FRUSTUM_SPLITS];
	UPartition gPartitionsU[NUM_FRUSTUM_SPLITS];
	BoundsUint gPartitionBoundsU[NUM_FRUSTUM_SPLITS];
};

void ComputePartitionDataFromBounds(in BoundsUint bounds, out vec3 scale, out vec3 bias);


void main(void)
{
	uint cascadeIndex = gl_LocalInvocationIndex;

	vec3 scale;
	vec3 bias;

	ComputePartitionDataFromBounds(gPartitionBoundsU[cascadeIndex], scale, bias);

	gPartitions[cascadeIndex].scale = scale;
	gPartitions[cascadeIndex].bias = bias;
}

void ComputePartitionDataFromBounds(in BoundsUint bounds, out vec3 scale, out vec3 bias)
{
	vec3 minTexCoord = vec3(uintBitsToFloat(bounds.minCoord.x), uintBitsToFloat(bounds.minCoord.y), uintBitsToFloat(bounds.minCoord.z));
	vec3 maxTexCoord = vec3(uintBitsToFloat(bounds.maxCoord.x), uintBitsToFloat(bounds.maxCoord.y), uintBitsToFloat(bounds.maxCoord.z));

	minTexCoord -= lightSpaceBorder.xyz;
	maxTexCoord += lightSpaceBorder.xyz;

	scale = 1.0 / (maxTexCoord - minTexCoord);
	bias = -minTexCoord * scale;

	float oneMinusTwoFactor = 1.0 - 2.0 * dilationFactor;
	scale *= oneMinusTwoFactor;
	bias = dilationFactor + oneMinusTwoFactor * bias;

	vec3 targetScale = min(scale, maxScale.xyz);

	//quantize
	//targetScale.xy = max(1.0, exp2(floor(log2(targetScale.xy))));

	vec3 center = vec3(0.5, 0.5, 0.5);
	bias = (targetScale / scale) * (bias - center) + center;
	scale = targetScale;

	//vec2 texelsInLightSpace = shadowMapSizes.xy;
	//bias.xy = floor(bias.xy * texelsInLightSpace) / texelsInLightSpace;

	if(maxTexCoord.x < minTexCoord.x || maxTexCoord.y < minTexCoord.y || maxTexCoord.z < minTexCoord.z)
	{
		scale = vec3(float(0x7F7FFFFF),float(0x7F7FFFFF), float(0x7F7FFFFF));
		bias = scale; 
	}
}