#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
#define REDUCE_ZBOUNDS_BLOCK_DIM 16
#define REDUCE_ZBOUNDS_BLOCK_SIZE (REDUCE_ZBOUNDS_BLOCK_DIM * REDUCE_ZBOUNDS_BLOCK_DIM)

#define MAX_NUM_DIRECTIONAL_LIGHTS 50
#define MAX_NUM_POINT_LIGHTS 4096
#define MAX_NUM_SPOT_LIGHTS MAX_NUM_POINT_LIGHTS
#define MAX_NUM_SHADOW_MAP_PAGES 50

#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_SPOT_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_POINT_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_SHADOW_MAP_PAGES

layout (local_size_x = REDUCE_ZBOUNDS_BLOCK_DIM, local_size_y = REDUCE_ZBOUNDS_BLOCK_DIM, local_size_z = 1) in;


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
    mat4 inverseProjection;
    mat4 inverseView;
    mat4 vpMatrix;
    mat4 prevProjection;
    mat4 prevView;
    mat4 prevVPMatrix;
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
	uvec4 tileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
	vec4 jitter;
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
	Tex2DAddress zPrePassShadowsSurface[2];
	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
};


layout (std140, binding = 3) uniform SDSMParams
{
	vec4 lightSpaceBorder;
	vec4 maxScale;
	vec4 projMultSplitScaleZMultLambda;
	float dilationFactor;
	uint scatterTileDim;
	uint reduceTileDim;
	uint padding;
};


//@NOTE(Serge): we can only have 4 cascades like this
struct Partition
{
	vec4 intervalBegin;
	vec4 intervalEnd;
};

struct UPartition
{
	uvec4 intervalBegin;
	uvec4 intervalEnd;
};


layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions;
	UPartition gPartitionsU;

	vec4 gScale[NUM_FRUSTUM_SPLITS][MAX_NUM_SHADOW_MAP_PAGES];
	vec4 gBias[NUM_FRUSTUM_SPLITS][MAX_NUM_SHADOW_MAP_PAGES];

	mat4 gShadowMatrix[MAX_NUM_SHADOW_MAP_PAGES];

	float gSpotLightShadowMapPages[NUM_SPOTLIGHT_SHADOW_PAGES];
	float gPointLightShadowMapPages[NUM_POINTLIGHT_SHADOW_PAGES];
	float gDirectionLightShadowMapPages[NUM_DIRECTIONLIGHT_SHADOW_PAGES];
};




shared float sMinZ[REDUCE_ZBOUNDS_BLOCK_SIZE];
shared float sMaxZ[REDUCE_ZBOUNDS_BLOCK_SIZE];

//vec3 ComputePositionViewFromZ(vec2 positionScreen, float viewSpaceZ);
float ComputeSurfaceDataPositionView(uvec2 coords, ivec2 depthBufferSize);

void main(void)
{
	ivec3 depthBufferSize = textureSize(sampler2DArray(zPrePassShadowsSurface[0].container), 0);

	float minZ = exposureNearFar.z;
	float maxZ = exposureNearFar.y;

	uvec2 tileStart = (gl_WorkGroupID.xy * uvec2(reduceTileDim, reduceTileDim)) + gl_LocalInvocationID.xy;
	for(uint tileY = 0; tileY < reduceTileDim; tileY += REDUCE_ZBOUNDS_BLOCK_DIM)
	{
		for(uint tileX = 0; tileX < reduceTileDim; tileX += REDUCE_ZBOUNDS_BLOCK_DIM)
		{
			uvec2 globalCoords = tileStart + uvec2(tileX, tileY);
			float positionViewZ = ComputeSurfaceDataPositionView(globalCoords, depthBufferSize.xy);

			if(positionViewZ >= exposureNearFar.y && positionViewZ < exposureNearFar.z)
			{
				minZ = min(minZ, positionViewZ);
				maxZ = max(maxZ, positionViewZ);
			}
		}
	}

	sMinZ[gl_LocalInvocationIndex] = minZ;
	sMaxZ[gl_LocalInvocationIndex] = maxZ;

	//Barrier
	barrier();
	memoryBarrierShared();
	
	

	for(uint offset = (REDUCE_ZBOUNDS_BLOCK_SIZE >> 1); offset > 0; offset >>= 1)
	{
		if(gl_LocalInvocationIndex < offset)
		{
			sMinZ[gl_LocalInvocationIndex] = min(sMinZ[gl_LocalInvocationIndex], sMinZ[offset + gl_LocalInvocationIndex]);
			sMaxZ[gl_LocalInvocationIndex] = max(sMaxZ[gl_LocalInvocationIndex], sMaxZ[offset + gl_LocalInvocationIndex]);
		}

		//Barrier
		
		barrier();
		memoryBarrierShared();
		
		
	}

	if(gl_LocalInvocationIndex == 0)
	{
		atomicMin(gPartitionsU.intervalBegin[0], floatBitsToUint(sMinZ[0]));
		atomicMax(gPartitionsU.intervalEnd[NUM_FRUSTUM_SPLITS - 1], floatBitsToUint(sMaxZ[NUM_FRUSTUM_SPLITS - 1]));
	}
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0-1.0; // back to NDC 
    float near = exposureNearFar.y;
    float far = exposureNearFar.z;

    return (2.0 * near * far) / (far + near - z * (far - near));	
}

float ComputeSurfaceDataPositionView(uvec2 coords, ivec2 depthBufferSize)
{
	vec3 texCoords = vec3(float(coords.x) / float(depthBufferSize.x), float(coords.y)/ float(depthBufferSize.y), zPrePassShadowsSurface[0].page);

	return LinearizeDepth(texture(sampler2DArray(zPrePassShadowsSurface[0].container), texCoords).r);
}

