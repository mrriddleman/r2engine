#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
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
};

#define MAX_CLUSTERS 4096 //hmm would like to get rid of this but I don't want to use too many SSBOs

layout (std430, binding=8) buffer Clusters
{
	uint numUniqueActiveClusters;
	bool activeClusters[MAX_CLUSTERS];
	uint uniqueActiveClusters[MAX_CLUSTERS]; //compacted list
	VolumeTileAABB clusters[MAX_CLUSTERS];
};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

uint GetClusterIndex(vec3 pixelCoord);
void MarkActiveCluster(uvec2 screenCoord);
uint GetDepthSlice(float z);
float LinearizeDepth(float depth);

//We could make this a compute shader instead?
void main()
{
	MarkActiveCluster(uvec2(gl_FragCoord.xy));
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0-1.0; // back to NDC 
    float near = exposureNearFar.y;
    float far = exposureNearFar.z;

    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void MarkActiveCluster(uvec2 screenCoord)
{
	vec2 texCoord = screenCoord / fovAspectResXResY.zw;

	float z = texture(sampler2DArray(zPrePassSurface.container), texCoord).r;

	uint clusterID = GetClusterIndex(vec3(screenCoord, z));

	activeClusters[clusterID] = true;
}

uint GetDepthSlice(float z)
{
	return uint(max(log2(LinearizeDepth(z)) * clusterScaleBias.x + clusterScaleBias.y, 0.0));
}

uint GetClusterIndex(vec3 pixelCoord)
{
	uint clusterZVal = GetDepthSlice(pixelCoord.z);

	uvec3 clusterID = uvec3(uvec2(pixelCoord.xy / clusterTileSizes.w), clusterZVal);

	return clusterID.x + clusterTileSizes.x * clusterID.y + (clusterTileSizes.x * clusterTileSizes.y) * clusterID.z;
}