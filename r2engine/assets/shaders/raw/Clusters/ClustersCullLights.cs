#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 16, local_size_y = 16, local_size_z = 2) in;

#define NUM_FRUSTUM_SPLITS 4
#define MAX_CLUSTERS 4096 //hmm would like to get rid of this but I don't want to use too many SSBOs

#define MAX_NUM_DIRECTIONAL_LIGHTS 50
#define MAX_NUM_POINT_LIGHTS 4096
#define MAX_NUM_SPOT_LIGHTS MAX_NUM_POINT_LIGHTS
#define MAX_NUM_SHADOW_MAP_PAGES 50
#define MAX_NUMBER_OF_LIGHTS_PER_CLUSTER 100

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
    uint pointLightOffset;
    uint pointLightCount;
    uint spotLightOffset;
    uint spotLightCount;
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
    mat4 inverseView;
    mat4 vpMatrix;
    mat4 prevProjection;
    mat4 prevView;
    mat4 prevVPMatrix;
};

layout (std430, binding = 4) buffer Lighting
{
	PointLight pointLights[MAX_NUM_POINT_LIGHTS];
	DirLight dirLights[MAX_NUM_DIRECTIONAL_LIGHTS];
	SpotLight spotLights[MAX_NUM_SPOT_LIGHTS];
	SkyLight skylight;

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int numPrefilteredRoughnessMips;
	int useSDSMShadows;

	int numShadowCastingDirectionLights;
	int numShadowCastingPointLights;
	int numShadowCastingSpotLights;

	int64_t shadowCastingDirectionLights[MAX_NUM_SHADOW_MAP_PAGES];
	int64_t shadowCastingPointLights[MAX_NUM_SHADOW_MAP_PAGES];
	int64_t shadowCastingSpotLights[MAX_NUM_SHADOW_MAP_PAGES];
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

shared uint visiblePointLightCount;
shared uint visibleSpotLightCount;
shared uint visiblePointLightIndices[MAX_NUM_POINT_LIGHTS];
shared uint visibleSpotLightIndices[MAX_NUM_SPOT_LIGHTS];

bool TestPointLightSphereAABB(uint light, uint tile);
bool TestSpotLightAABB(uint light, uint tile);
float SquareDistPointAABB(vec3 point, uint tile);

void main()
{
	//get the tile index for this global invocation
	uint tileIndex = uniqueActiveClusters[gl_WorkGroupID.x]; //we use a 1d array for the invocations in x
	uint localThreadCount = gl_WorkGroupSize.x * gl_WorkGroupSize.y;
	uint numPointLightBatches = (numPointLights + localThreadCount - 1) / localThreadCount;
	uint numSpotLightBatches = (numSpotLights + localThreadCount - 1) / localThreadCount;

	visiblePointLightCount = 0;
	visibleSpotLightCount = 0;

	barrier();
	memoryBarrierShared();

	uint localIndex = (gl_LocalInvocationID.x + gl_LocalInvocationID.y * gl_WorkGroupSize.x);
	
	if(gl_LocalInvocationID.z == 0)
	{
		for(uint batch = 0; batch < numPointLightBatches; ++batch)
		{
			uint lightIndex = batch * localThreadCount + localIndex;
			
			if(lightIndex < numPointLights && TestPointLightSphereAABB(lightIndex, tileIndex))
			{
				uint offset = atomicAdd(visiblePointLightCount, 1);
				visiblePointLightIndices[offset] = lightIndex;
			}
		}
	}

	if(gl_LocalInvocationID.z == 1)
	{
		for(uint batch = 0; batch < numSpotLightBatches; ++batch)
		{
			uint lightIndex = batch * localThreadCount + localIndex;

			if(lightIndex < numSpotLights && TestSpotLightAABB(lightIndex, tileIndex) )
			{
				uint offset = atomicAdd(visibleSpotLightCount, 1);
				visibleSpotLightIndices[offset] = lightIndex;
			}
		}
	}
	
	barrier();
	memoryBarrierShared();

	if(gl_LocalInvocationIndex == 0)
	{
		uint offset = atomicAdd(globalLightIndexCount.x, visiblePointLightCount);

		for(uint i = 0; i < visiblePointLightCount; ++i)
		{
			globalLightIndexList[offset + i].x = visiblePointLightIndices[i];
		}

		lightGrid[tileIndex].pointLightOffset = offset;
		lightGrid[tileIndex].pointLightCount = visiblePointLightCount;


		uint offset2 = atomicAdd(globalLightIndexCount.y, visibleSpotLightCount);

		for(uint i = 0; i < visibleSpotLightCount; ++i)
		{
			globalLightIndexList[offset2 + i].y = visibleSpotLightIndices[i];
		}

		lightGrid[tileIndex].spotLightOffset = offset2;
		lightGrid[tileIndex].spotLightCount = visibleSpotLightCount;
	}
	

}

bool TestPointLightSphereAABB(uint light, uint tile)
{
 	float radiusSq = 1.0 / pointLights[light].lightProperties.fallOffRadius;

    vec3 center  = vec3(view * pointLights[light].position);

    float squaredDistance = SquareDistPointAABB(center, tile);

    return squaredDistance <= radiusSq;
}

bool TestSpotLightAABB(uint light, uint tile)
{
	float radiusSq = 1.0 / spotLights[light].lightProperties.fallOffRadius;

	vec3 center = vec3(view * vec4(spotLights[light].position.xyz, 1));

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
