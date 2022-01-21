#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
#define REDUCE_BOUNDS_BLOCK_X 16
#define REDUCE_BOUNDS_BLOCK_Y 8
#define REDUCE_BOUNDS_BLOCK_SIZE (REDUCE_BOUNDS_BLOCK_X*REDUCE_BOUNDS_BLOCK_Y)
#define REDUCE_BOUNDS_SHARED_MEMORY_ARRAY_SIZE (NUM_FRUSTUM_SPLITS * REDUCE_BOUNDS_BLOCK_SIZE)
#define MAX_NUM_LIGHTS 50

shared vec3 sBoundsMin[REDUCE_BOUNDS_SHARED_MEMORY_ARRAY_SIZE];
shared vec3 sBoundsMax[REDUCE_BOUNDS_SHARED_MEMORY_ARRAY_SIZE];

layout (local_size_x = REDUCE_BOUNDS_BLOCK_X, local_size_y = REDUCE_BOUNDS_BLOCK_Y, local_size_z = 1) in;


// Uint version of the bounds structure for atomic usage
// NOTE: This version cannot represent negative numbers!
struct BoundsUint
{
    uvec3 minCoord;
    uvec3 maxCoord;
};

// Reset bounds to [0, maxFloat]
BoundsUint EmptyBoundsUint()
{
    BoundsUint b;
    b.minCoord = uvec3(uint(0x7F7FFFFF));      // Float max
    b.maxCoord = uvec3(0);               // NOTE: Can't be negative!!
    return b;
}

// Float version of structure for convenient
// NOTE: We still tend to maintain the non-negative semantics of the above for consistency
struct BoundsFloat
{
    vec3 minCoord;
    vec3 maxCoord;
};

struct Partition
{
	float intervalBegin;
	float intervalEnd;

	vec3 scale;
	vec3 bias;
};

struct UPartition
{
	uint intervalBegin;
	uint intervalEnd;

	vec3 scale;
	vec3 bias;
};

struct SurfaceData
{
	vec3 positionView;
	vec3 lightTexCoord;
};

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct LightProperties
{
	vec4 color;
	float fallOffRadius;
	float intensity;
//	uint32_t castsShadows;
	int64_t lightID;
};

struct LightSpaceMatrixData
{
	mat4 lightViewMatrices[NUM_FRUSTUM_SPLITS];
	mat4 lightProjMatrices[NUM_FRUSTUM_SPLITS];
};

struct PointLight
{
	LightProperties lightProperties;
	vec4 position;
};

struct DirLight
{
	LightProperties lightProperties;
	vec4 direction;	

	mat4 cameraViewToLightProj;
	LightSpaceMatrixData lightSpaceMatrixData;
};

struct SpotLight
{
	LightProperties lightProperties;
	vec4 position;//w is radius
	vec4 direction;//w is cutoff
};

struct SkyLight
{
	LightProperties lightProperties;
	Tex2DAddress diffuseIrradianceTexture;
	Tex2DAddress prefilteredRoughnessTexture;
	Tex2DAddress lutDFGTexture;
//	int numPrefilteredRoughnessMips;
};

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
};

//@NOTE(Serge): this is in the order of the render target surfaces in RenderTarget.h
layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
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

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions[NUM_FRUSTUM_SPLITS];
	UPartition gPartitionsU[NUM_FRUSTUM_SPLITS];
	BoundsUint gPartitionBoundsU[NUM_FRUSTUM_SPLITS];
};

BoundsFloat BoundsUintToFloat(BoundsUint u)
{
    BoundsFloat f;
    f.minCoord.x = float(u.minCoord.x);
    f.minCoord.y = float(u.minCoord.y);
    f.minCoord.z = float(u.minCoord.z);
    f.maxCoord.x = float(u.maxCoord.x);
    f.maxCoord.y = float(u.maxCoord.y);
    f.maxCoord.z = float(u.maxCoord.z);
    return f;
}

BoundsFloat EmptyBoundsFloat()
{
    return BoundsUintToFloat(EmptyBoundsUint());
}


SurfaceData ComputeSurfaceData(uvec2 coords, ivec2 depthBufferSize);

vec3 ProjectIntoLightTexCoord(vec3 positionView)
{
	vec4 positionLight = dirLights[0].cameraViewToLightProj * vec4(positionView, 1.0);
	vec3 texCoord = (positionLight.xyz / positionLight.w) * vec3(0.5, 0.5, 1.0) + vec3(0.5, 0.5, 0.0);
	return texCoord;
}

vec3 ComputePositionViewFromZ(vec2 positionScreen, float viewSpaceZ)
{
	vec2 screenSpaceRay = vec2(positionScreen.x / projection[0][0], positionScreen.y / projection[1][1]);
	return vec3(screenSpaceRay.x * viewSpaceZ, screenSpaceRay.y * viewSpaceZ, viewSpaceZ);
}


void main(void)
{

	ivec3 depthBufferSize = textureSize(sampler2DArray(zPrePassSurface.container), 0);
	

	BoundsFloat boundsReduce[NUM_FRUSTUM_SPLITS];

	for(uint part = 0; part < NUM_FRUSTUM_SPLITS; ++part)
	{
		boundsReduce[part] = EmptyBoundsFloat();
	}

	float nearZ = gPartitions[0].intervalBegin;
	float farZ = gPartitions[NUM_FRUSTUM_SPLITS - 1].intervalEnd;

	uvec2 tileStart = gl_WorkGroupID.xy * reduceTileDim + gl_LocalInvocationID.xy;
	for(uint tileY = 0; tileY < reduceTileDim; tileY += REDUCE_BOUNDS_BLOCK_Y)
	{
		for(uint tileX = 0; tileX < reduceTileDim; tileX += REDUCE_BOUNDS_BLOCK_X)
		{
			uvec2 globalCoords = tileStart + uvec2(tileX, tileY);

			SurfaceData surfaceData = ComputeSurfaceData(globalCoords, depthBufferSize.xy);

			if(surfaceData.positionView.z >= nearZ && surfaceData.positionView.z < farZ)
			{
				uint part = 0;
				for(uint i = 0; i <(NUM_FRUSTUM_SPLITS - 1); ++i)
				{
					if(surfaceData.positionView.z >= gPartitions[i].intervalEnd)
					{
						++part;
					}
				}

				boundsReduce[part].minCoord = min(boundsReduce[part].minCoord, surfaceData.lightTexCoord.xyz);
				boundsReduce[part].maxCoord = max(boundsReduce[part].maxCoord, surfaceData.lightTexCoord.xyz);
			}
		}
	}


	for(uint part = 0; part < NUM_FRUSTUM_SPLITS; ++part)
	{
		uint index = gl_LocalInvocationIndex * NUM_FRUSTUM_SPLITS + part;
		sBoundsMin[index] = boundsReduce[part].minCoord;
		sBoundsMax[index] = boundsReduce[part].maxCoord;
	}

	barrier();
	memoryBarrierShared();

	for(uint offset = (REDUCE_BOUNDS_SHARED_MEMORY_ARRAY_SIZE >> 1); offset >= NUM_FRUSTUM_SPLITS; offset >>= 1)
	{
		for(uint i = gl_LocalInvocationIndex; i < offset; i += REDUCE_BOUNDS_BLOCK_SIZE)
		{
			sBoundsMin[i] = min(sBoundsMin[i], sBoundsMin[offset + i]);
			sBoundsMax[i] = max(sBoundsMax[i], sBoundsMax[offset + i]);
		}
		barrier();
		memoryBarrierShared();
	}

	if(gl_LocalInvocationIndex < NUM_FRUSTUM_SPLITS)
	{
		atomicMin(gPartitionBoundsU[gl_LocalInvocationIndex].minCoord.x, floatBitsToUint(sBoundsMin[gl_LocalInvocationIndex].x));
		atomicMin(gPartitionBoundsU[gl_LocalInvocationIndex].minCoord.y, floatBitsToUint(sBoundsMin[gl_LocalInvocationIndex].y));
		atomicMin(gPartitionBoundsU[gl_LocalInvocationIndex].minCoord.z, floatBitsToUint(sBoundsMin[gl_LocalInvocationIndex].z));

		atomicMax(gPartitionBoundsU[gl_LocalInvocationIndex].minCoord.x, floatBitsToUint(sBoundsMax[gl_LocalInvocationIndex].x));
		atomicMax(gPartitionBoundsU[gl_LocalInvocationIndex].minCoord.y, floatBitsToUint(sBoundsMax[gl_LocalInvocationIndex].y));
		atomicMax(gPartitionBoundsU[gl_LocalInvocationIndex].minCoord.z, floatBitsToUint(sBoundsMax[gl_LocalInvocationIndex].z));
	}
}

SurfaceData ComputeSurfaceData(uvec2 coords, ivec2 depthBufferSize)
{
	vec3 texCoords = vec3(float(coords.x) / float(depthBufferSize.x), float(coords.y)/ float(depthBufferSize.y), zPrePassSurface.page);

	//@NOTE(Serge): if we change our z pre pass to use projection, we should do an unproject here
	//return UnprojectDepthBufferZValue(texture(sampler2DArray(zPrePassSurface.container), texCoords).z);

	float viewSpaceZ = texture(sampler2DArray(zPrePassSurface.container), texCoords).r;

	vec2 screenPosition = vec2((2.0 *float(coords.x)) / float(depthBufferSize.x), (2.0*float(coords.y))/ float(depthBufferSize.y)) + vec2(-1.0, -1.0);

	SurfaceData data;

	data.positionView = ComputePositionViewFromZ(screenPosition, viewSpaceZ);
	data.lightTexCoord = ProjectIntoLightTexCoord(data.positionView);

	return data;
}