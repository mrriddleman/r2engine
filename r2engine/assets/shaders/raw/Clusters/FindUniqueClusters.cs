#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 2) in;

#include "Common/Defines.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/ClusterData.glsl"
#include "Input/ShaderBufferObjects/DispatchIndirectData.glsl"

void main()
{
	uint numLocalThreads = (gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z);
	uint globalIndex = gl_WorkGroupID.x + gl_WorkGroupID.y * (gl_NumWorkGroups.x) + gl_WorkGroupID.z * (gl_NumWorkGroups.x * gl_NumWorkGroups.y);
	uint clusterIndex = numLocalThreads * globalIndex + gl_LocalInvocationIndex;

	if(activeClusters[clusterIndex])
	{
		uint offset = atomicAdd(dispatchCMDs[0].numGroupsX, 1);
		uniqueActiveClusters[offset] = clusterIndex;
	}

	dispatchCMDs[0].numGroupsY = 1;
	dispatchCMDs[0].numGroupsZ = 1;
}