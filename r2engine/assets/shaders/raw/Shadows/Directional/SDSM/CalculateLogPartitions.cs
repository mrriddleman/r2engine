#version 450 core
#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Common/Texture.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/SDSMParams.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;


float LogPartitionFromRange(uint part, float minZ, float maxZ);

void main(void)
{
	float minZ = uintBitsToFloat(gPartitionsU.intervalBegin[0]);
	float maxZ = uintBitsToFloat(gPartitionsU.intervalEnd[NUM_FRUSTUM_SPLITS-1]);

	uint cascadeIndex = gl_LocalInvocationIndex;

	gPartitions.intervalBegin[cascadeIndex] = LogPartitionFromRange(cascadeIndex, minZ, maxZ);
	gPartitions.intervalEnd[cascadeIndex] = LogPartitionFromRange(cascadeIndex + 1, minZ, maxZ);
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

		float lambda = projMultSplitScaleZMultLambda.w;
		float ratio = maxZ / minZ;
		float power = float(part + 1) * (1.0 / float(NUM_FRUSTUM_SPLITS));
		float logScale = minZ * pow(abs(ratio), power);
		float uniformScale = minZ + range * power;
		float d = lambda * (logScale - uniformScale) + uniformScale;
		z =  (d - nearClip) / clipRange;

	}

	return z;
}