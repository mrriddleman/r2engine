#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 20, local_size_y = 20) in;

#define NUM_FRUSTUM_SPLITS 4
#define MAX_CLUSTERS 4096 //hmm would like to get rid of this but I don't want to use too many SSBOs
#define MAX_NUMBER_OF_LIGHTS_PER_CLUSTER 100

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

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

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
    mat4 invProjection;
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

//@NOTE(Serge): this is in the order of the render target surfaces in RenderTarget.h
layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
	Tex2DAddress ambientOcclusionDenoiseSurface;
	Tex2DAddress zPrePassShadowSurface;
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

uint GetClusterIndex(vec3 pixelCoord);
void MarkActiveCluster(uvec2 screenCoord);
uint GetDepthSlice(float z);
float LinearizeDepth(float depth);

//We could make this a compute shader instead?
void main()
{
	//assuming that we use the global dispatch to be the screen size

	uvec2 screenCoord;
	screenCoord.x = gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
	screenCoord.y = gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y;

	MarkActiveCluster(screenCoord); 
}

void MarkActiveCluster(uvec2 screenCoord)
{
	vec3 texCoord = vec3(screenCoord / fovAspectResXResY.zw, zPrePassSurface.page);

	float z = texture(sampler2DArray(zPrePassSurface.container), texCoord).r;

	uint clusterID = GetClusterIndex(vec3(screenCoord, z));

	activeClusters[clusterID] = true;
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0-1.0; // back to NDC 
    float near = exposureNearFar.y;
    float far = exposureNearFar.z;

    return (2.0 * near * far) / (far + near - z * (far - near));	
}

uint GetDepthSlice(float z)
{
	return uint(max(floor(log2(LinearizeDepth(z)) * clusterScaleBias.x + clusterScaleBias.y), 0.0));
}

uint GetClusterIndex(vec3 pixelCoord)
{
	uint clusterZVal = GetDepthSlice(pixelCoord.z);

	uvec3 clusterID = uvec3(uvec2(pixelCoord.xy / clusterTileSizes.w), clusterZVal);

	return clusterID.x + clusterTileSizes.x * clusterID.y + (clusterTileSizes.x * clusterTileSizes.y) * clusterID.z;
}