#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#define NUM_FRUSTUM_SPLITS 4
#define MAX_CLUSTERS 4096 //hmm would like to get rid of this but I don't want to use too many SSBOs
const uint MAX_NUM_LIGHTS = 50;
const uint NUM_SIDES_FOR_POINTLIGHT = 6;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

struct LightProperties
{
	vec4 color;
	uvec4 castsShadowsUseSoftShadows;
	float fallOffRadius;
	float intensity;
	//uint32_t castsShadows;
	int64_t lightID;
};

struct LightSpaceMatrixData
{
	mat4 lightViewMatrices[NUM_FRUSTUM_SPLITS];
	mat4 lightProjMatrices[NUM_FRUSTUM_SPLITS];
};

struct DirLight
{
//	
	LightProperties lightProperties;
	vec4 direction;
	mat4 cameraViewToLightProj;
	LightSpaceMatrixData lightSpaceMatrixData;
};


struct PointLight
{
	LightProperties lightProperties;
	vec4 position;

	mat4 lightSpaceMatrices[NUM_SIDES_FOR_POINTLIGHT];
};

struct SpotLight
{
	LightProperties lightProperties;
	vec4 position;//w is radius
	vec4 direction;//w is cutoff

	mat4 lightSpaceMatrix;
};

struct SkyLight
{
	LightProperties lightProperties;
	Tex2DAddress diffuseIrradianceTexture;
	Tex2DAddress prefilteredRoughnessTexture;
	Tex2DAddress lutDFGTexture;
//	int numPrefilteredRoughnessMips;
};


struct LightGrid{
    uint offset;
    uint count;
    uint pad0;
    uint pad1;
};

struct VolumeTileAABB{
    vec4 minPoint;
    vec4 maxPoint;
};

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
    mat4 invProjection;
};

layout (std430, binding = 4) buffer Lighting
{
	PointLight pointLights[MAX_NUM_LIGHTS];
	DirLight dirLights[MAX_NUM_LIGHTS];
	SpotLight spotLights[MAX_NUM_LIGHTS];
	SkyLight skylight;

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int numPrefilteredRoughnessMips;
	int useSDSMShadows;
};

layout (std430, binding=8) buffer Clusters
{
	uint globalLightIndexCount;
	uint globalLightIndexList[MAX_NUM_LIGHTS];
	bool activeClusters[MAX_CLUSTERS];
	uint uniqueActiveClusters[MAX_CLUSTERS]; //compacted list of clusterIndices
	LightGrid lightGrid[MAX_CLUSTERS];
	VolumeTileAABB clusters[MAX_CLUSTERS];
};

shared uint visibleLightCount;
shared uint visibleLightIndices[MAX_NUM_LIGHTS];

bool TestSphereAABB(uint light, uint tile);
float SquareDistPointAABB(vec3 point, uint tile);

void main()
{
	//get the tile index for this global invocation
	uint tileIndex = uniqueActiveClusters[gl_GlobalInvocationID.x]; //we use a 1d array for the invocations in x
	uint localThreadCount = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
	uint numBatches = (numPointLights + localThreadCount - 1) / localThreadCount;

	visibleLightCount = 0;
	memoryBarrierShared();
	barrier();

	for(uint batch = 0; batch < numBatches; ++batch)
	{
		uint lightIndex = batch * localThreadCount + gl_LocalInvocationIndex;
		
		if(lightIndex < numPointLights && TestSphereAABB(lightIndex, tileIndex))
		{
			uint offset = atomicAdd(visibleLightCount, 1);
			visibleLightIndices[offset] = lightIndex;
			
			memoryBarrierShared();
		}
	}

	memoryBarrierShared();
	barrier();

	if(gl_LocalInvocationIndex == 0)
	{
		uint offset = atomicAdd(globalLightIndexCount, visibleLightCount);

		for(uint i = 0; i < visibleLightCount; ++i)
		{
			globalLightIndexList[offset + i] = visibleLightIndices[i];
		}

		lightGrid[tileIndex].offset = offset;
		lightGrid[tileIndex].count = visibleLightCount;
	}
}

bool TestSphereAABB(uint light, uint tile)
{
 	float radiusSq = 1.0 / pointLights[light].lightProperties.fallOffRadius;
    vec3 center  = vec3(view * pointLights[light].position);
    float squaredDistance = SquareDistPointAABB(center, tile);

    return squaredDistance <= radiusSq;
}

float SquareDistPointAABB(vec3 point, uint tile)
{
	float sqDist = 0.0;
    VolumeTileAABB currentCell = clusters[tile];
    clusters[tile].maxPoint[3] = tile;

    for(int i = 0; i < 3; ++i){
        float v = point[i];
        if(v < currentCell.minPoint[i]){
            sqDist += (currentCell.minPoint[i] - v) * (currentCell.minPoint[i] - v);
        }
        if(v > currentCell.maxPoint[i]){
            sqDist += (v - currentCell.maxPoint[i]) * (v - currentCell.maxPoint[i]);
        }
    }

    return sqDist;
}