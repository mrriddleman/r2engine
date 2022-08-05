#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 8, local_size_y = 8, local_size_z = 2) in;

#define MAX_CLUSTERS 4096 //hmm would like to get rid of this but I don't want to use too many SSBOs
const uint MAX_NUM_LIGHTS = 50;

struct VolumeTileAABB
{
	vec4 minPoint;
	vec4 maxPoint;
};

struct LightGrid{
    uint pointLightOffset;
    uint pointLightCount;
    uint spotLightOffset;
    uint spotLightCount;
};

struct DispatchIndirectCommand
{
	uint numGroupsX;
	uint numGroupsY;
	uint numGroupsZ;
	uint unused;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspectResXResY;
    uint64_t frame;
    vec2 clusterScaleBias;
    uvec4 clusterTileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}

};

layout (std430, binding=8) buffer Clusters
{
	uvec2 globalLightIndexCount;
	uvec2 globalLightIndexList[(MAX_NUM_LIGHTS / 2) * MAX_CLUSTERS];
	bool activeClusters[MAX_CLUSTERS];
	uint uniqueActiveClusters[MAX_CLUSTERS]; //compacted list of clusterIndices
	LightGrid lightGrid[MAX_CLUSTERS];
	VolumeTileAABB clusters[MAX_CLUSTERS];
};

layout (std430, binding=9) buffer DispatchIndirectCommands
{
	DispatchIndirectCommand dispatchCMDs[];
};

//uint GetClusterIndex(uvec3 tileID);

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

// uint GetClusterIndex(uvec3 clusterID)
// {
// 	return clusterID.x + clusterTileSizes.x * clusterID.y + (clusterTileSizes.x * clusterTileSizes.y) * clusterID.z;
// }