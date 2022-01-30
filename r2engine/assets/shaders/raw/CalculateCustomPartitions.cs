#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;

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

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspect;
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
};

void ComputePartitionDataFromBounds(vec3 minCoord, vec3 maxCoord, inout vec3 scale, inout vec3 bias);


void main(void)
{
	uint cascadeIndex = gl_LocalInvocationIndex;

	vec3 scale = vec3(0);
	vec3 bias = vec3(0);

	uint umaxX = gPartitionsU[cascadeIndex].intervalEndMaxCoord.y;
	uint uminX = gPartitionsU[cascadeIndex].intervalBeginMinCoord.y;
	uint umaxY = gPartitionsU[cascadeIndex].intervalEndMaxCoord.z; 
	uint uminY = gPartitionsU[cascadeIndex].intervalBeginMinCoord.z;
	uint umaxZ = gPartitionsU[cascadeIndex].intervalEndMaxCoord.w;
	uint uminZ = gPartitionsU[cascadeIndex].intervalBeginMinCoord.w;

	float fminX = uintBitsToFloat(uminX);
	float fminY = uintBitsToFloat(uminY);
	float fminZ = uintBitsToFloat(uminZ);

	float fmaxX = uintBitsToFloat(umaxX);
	float fmaxY = uintBitsToFloat(umaxY);
	float fmaxZ = uintBitsToFloat(umaxZ);

	vec3 minCoord;
	minCoord.x = fminX;
	minCoord.y = fminY;
	minCoord.z = fminZ;

	vec3 maxCoord;
	maxCoord.x = fmaxX;
	maxCoord.y = fmaxY;
	maxCoord.z = fmaxZ;



	ComputePartitionDataFromBounds(minCoord, maxCoord, scale, bias);

	gPartitions[cascadeIndex].intervalBeginScale.y = scale.x;
	gPartitions[cascadeIndex].intervalBeginScale.z = scale.y;
	gPartitions[cascadeIndex].intervalBeginScale.w = scale.z;

	gPartitions[cascadeIndex].intervalEndBias.y = bias.x;
	gPartitions[cascadeIndex].intervalEndBias.z = bias.y;
	gPartitions[cascadeIndex].intervalEndBias.w = bias.z;
}

void ComputePartitionDataFromBounds(vec3 minCoord, vec3 maxCoord, inout vec3 scale, inout vec3 bias)
{

	vec3 minTexCoord = minCoord;
	vec3 maxTexCoord = maxCoord;

	minTexCoord -= lightSpaceBorder.xyz;
	maxTexCoord += lightSpaceBorder.xyz;

	scale = 1.0/ maxTexCoord - minTexCoord;

	//scale = (maxTexCoord - minTexCoord);
	bias = -minTexCoord * scale;

	float oneMinusTwoFactor = 1.0 - 2.0 * dilationFactor;
	scale *= oneMinusTwoFactor;
	bias = dilationFactor + oneMinusTwoFactor * bias;

	vec3 targetScale = min(scale, maxScale.xyz);

	// //quantize
	// //targetScale.xy = max(1.0, exp2(floor(log2(targetScale.xy))));

	vec3 center = vec3(0.5, 0.5, 0.5);
	bias = (targetScale / scale) * (bias - center) + center;
	scale = targetScale;

	//scale = vec3(1);
	//bias = vec3(0);

	//bias = vec3(0);
	// //vec2 texelsInLightSpace = shadowMapSizes.xy;
	// //bias.xy = floor(bias.xy * texelsInLightSpace) / texelsInLightSpace;

//	if(maxTexCoord.x < minTexCoord.x || maxTexCoord.y < minTexCoord.y || maxTexCoord.z < minTexCoord.z)
	{
	//	scale = vec3(float(0x7F7FFFFF),float(0x7F7FFFFF), float(0x7F7FFFFF));
	//	bias = scale; 
	}
}