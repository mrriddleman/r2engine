#ifndef GLSL_CLUSTERS
#define GLSL_CLUSTERS

#define MAX_CLUSTERS 4096 //hmm would like to get rid of this but I don't want to use too many SSBOs
#define MAX_NUMBER_OF_LIGHTS_PER_CLUSTER 100

//for Tex2DAddress
#include "Common/Texture.glsl"
//for LinearizeDepth
#include "Depth/DepthUtils.glsl"

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

layout (std430, binding=8) buffer Clusters
{
	uvec2 globalLightIndexCount;
	uvec2 globalLightIndexList[MAX_NUMBER_OF_LIGHTS_PER_CLUSTER * MAX_CLUSTERS];
	bool activeClusters[MAX_CLUSTERS];
	uint uniqueActiveClusters[MAX_CLUSTERS]; //compacted list of clusterIndices
	LightGrid lightGrid[MAX_CLUSTERS];
	VolumeTileAABB clusters[MAX_CLUSTERS];
};

uint GetDepthSlice(float z, vec2 nearFar, vec2 scaleBias)
{
	return uint(max(floor(log2(LinearizeDepth(z, nearFar.x, nearFar.y)) * scaleBias.x + scaleBias.y), 0.0));
}

uint GetClusterIndex(vec2 pixelCoord, vec2 resolution, vec2 nearFar, vec2 scaleBias, uvec4 tileSizes, Tex2DAddress zTexture)
{
	vec3 texCoord = vec3(pixelCoord / resolution, zTexture.page);

	float z =  texture(sampler2DArray(zTexture.container), texCoord).r;

	uint clusterZVal = GetDepthSlice(z, nearFar, scaleBias);

	uvec3 clusterID = uvec3(uvec2(pixelCoord.xy / tileSizes.w), clusterZVal);

	return clusterID.x + tileSizes.x * clusterID.y + (tileSizes.x * tileSizes.y) * clusterID.z;
}

#endif

