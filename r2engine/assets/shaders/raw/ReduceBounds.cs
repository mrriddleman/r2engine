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

// Float version of structure for convenient
// NOTE: We still tend to maintain the non-negative semantics of the above for consistency
struct BoundsFloat
{
    vec3 minCoord;
    vec3 maxCoord;
};

struct Partition
{
	vec4 intervalBeginScale;
	vec4 intervalEndBias;
};

struct UPartition
{
	uvec4 intervalBeginMinCoord;
	uvec4 intervalEndMaxCoord;
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
	uvec4 castsShadowsUseSoftShadows;
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

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspect;
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
	mat4 gShadowMatrix;
};

BoundsFloat EmptyBoundsFloat()
{
	BoundsFloat bf;

	bf.minCoord = vec3(0x7F7FFFFF, 0x7F7FFFFF, 0x7F7FFFFF);
	bf.maxCoord = vec3(0, 0, 0);

	return bf;
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

	vec3 positionView;
	positionView.z = viewSpaceZ;
	positionView.xy = screenSpaceRay.xy * positionView.z;

	return positionView;
}


void main(void)
{
	ivec3 depthBufferSize = textureSize(sampler2DArray(zPrePassSurface.container), 0);
	

	BoundsFloat boundsReduce[NUM_FRUSTUM_SPLITS];

	for(uint part = 0; part < NUM_FRUSTUM_SPLITS; ++part)
	{
		boundsReduce[part] = EmptyBoundsFloat();
	}

	float nearZ = gPartitions[0].intervalBeginScale.x;
	float farZ = gPartitions[NUM_FRUSTUM_SPLITS - 1].intervalEndBias.x;

	uvec2 tileStart = gl_WorkGroupID.xy * uvec2(reduceTileDim) + gl_LocalInvocationID.xy;
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
	 				if(surfaceData.positionView.z >= gPartitions[i].intervalEndBias.x)
	 				{
	 					++part;
	 				}
	 			}

	 			boundsReduce[part].minCoord = min(boundsReduce[part].minCoord, surfaceData.lightTexCoord);//min(boundsReduce[part].minCoord, vec3(surfaceData.lightTexCoord.xyz));
	 			boundsReduce[part].maxCoord = max(boundsReduce[part].maxCoord, surfaceData.lightTexCoord);//max(boundsReduce[part].maxCoord, vec3(surfaceData.lightTexCoord.xyz));
	 		}
	 	}
	}


	for(uint p = 0; p < NUM_FRUSTUM_SPLITS; ++p)
	{
	 	uint index = gl_LocalInvocationIndex * NUM_FRUSTUM_SPLITS + p;
	 	sBoundsMin[index] = boundsReduce[p].minCoord.xyz;
	 	sBoundsMax[index] = boundsReduce[p].maxCoord.xyz;
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

		atomicMin(gPartitionsU[gl_LocalInvocationIndex].intervalBeginMinCoord.y, floatBitsToUint(sBoundsMin[gl_LocalInvocationIndex].x));
		atomicMin(gPartitionsU[gl_LocalInvocationIndex].intervalBeginMinCoord.z, floatBitsToUint(sBoundsMin[gl_LocalInvocationIndex].y));
		atomicMin(gPartitionsU[gl_LocalInvocationIndex].intervalBeginMinCoord.w, floatBitsToUint(sBoundsMin[gl_LocalInvocationIndex].z));

		atomicMax(gPartitionsU[gl_LocalInvocationIndex].intervalEndMaxCoord.y, floatBitsToUint(sBoundsMax[gl_LocalInvocationIndex].x));
		atomicMax(gPartitionsU[gl_LocalInvocationIndex].intervalEndMaxCoord.z, floatBitsToUint(sBoundsMax[gl_LocalInvocationIndex].y));
		atomicMax(gPartitionsU[gl_LocalInvocationIndex].intervalEndMaxCoord.w, floatBitsToUint(sBoundsMax[gl_LocalInvocationIndex].z));
	}
}

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    float near = exposureNearFar.y;
    float far = exposureNearFar.z;
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

SurfaceData ComputeSurfaceData(uvec2 coords, ivec2 depthBufferSize)
{
	vec3 texCoords = vec3(float(coords.x) / float(depthBufferSize.x), float(coords.y)/ float(depthBufferSize.y), zPrePassSurface.page);

	//@NOTE(Serge): if we change our z pre pass to use projection, we should do an unproject here
	float viewSpaceZ = LinearizeDepth(texture(sampler2DArray(zPrePassSurface.container), texCoords).r);

	vec2 screenPixelOffset = vec2(2.0, 2.0) / vec2(float(depthBufferSize.x), float(depthBufferSize.y));
	vec2 screenPosition = vec2(coords) * screenPixelOffset + vec2(-1.0, -1.0); //vec2(float(coords.x) / float(depthBufferSize.x), float(coords.y)/ float(depthBufferSize.y)) * 2.0 + vec2(-1.0, -1.0);

	SurfaceData data;

	data.positionView = ComputePositionViewFromZ(screenPosition, viewSpaceZ);
	data.lightTexCoord = ProjectIntoLightTexCoord(data.positionView);

	return data;
}