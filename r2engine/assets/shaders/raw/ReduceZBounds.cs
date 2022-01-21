#version 450 core
#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
#define REDUCE_ZBOUNDS_BLOCK_DIM 16
#define REDUCE_ZBOUNDS_BLOCK_SIZE (REDUCE_ZBOUNDS_BLOCK_DIM * REDUCE_ZBOUNDS_BLOCK_DIM)


layout (local_size_x = REDUCE_ZBOUNDS_BLOCK_DIM, local_size_y = REDUCE_ZBOUNDS_BLOCK_DIM, local_size_z = 1) in;


struct Tex2DAddress
{
	uint64_t  container;
	float page;
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

struct BoundsUint
{
    uvec3 minCoord;
    uvec3 maxCoord;
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

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions[NUM_FRUSTUM_SPLITS];
	UPartition gPartitionsU[NUM_FRUSTUM_SPLITS];
	BoundsUint gPartitionBoundsU[NUM_FRUSTUM_SPLITS];
};


shared float sMinZ[REDUCE_ZBOUNDS_BLOCK_SIZE];
shared float sMaxZ[REDUCE_ZBOUNDS_BLOCK_SIZE];

//vec3 ComputePositionViewFromZ(vec2 positionScreen, float viewSpaceZ);
float ComputeSurfaceDataPositionView(uvec2 coords, ivec2 depthBufferSize);

void main(void)
{
	ivec3 depthBufferSize = textureSize(sampler2DArray(zPrePassSurface.container), 0);

	float minZ = exposureNearFar.z;
	float maxZ = exposureNearFar.y;

	uvec2 tileStart = (gl_WorkGroupID.xy * reduceTileDim) + gl_LocalInvocationID.xy;
	for(uint tileY = 0; tileY < reduceTileDim; tileY += REDUCE_ZBOUNDS_BLOCK_DIM)
	{
		for(uint tileX = 0; tileX < reduceTileDim; tileX += REDUCE_ZBOUNDS_BLOCK_DIM)
		{
			uvec2 globalCoords = tileStart + uvec2(tileX, tileY);
			float positionViewZ = ComputeSurfaceDataPositionView(globalCoords, depthBufferSize.xy);
			//uint uintPositionViewZ = floatBitsToUint(positionViewZ);

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
		atomicMin(gPartitionsU[0].intervalBegin, floatBitsToUint(sMinZ[0]));
		atomicMax(gPartitionsU[NUM_FRUSTUM_SPLITS - 1].intervalEnd, floatBitsToUint(sMaxZ[0]));
	}
}

float UnprojectDepthBufferZValue(float zBuffer)
{
	//@TODO(Serge): not sure if this is correct...
	return projection[2][3] / (zBuffer - projection[2][2]);
}

float ComputeSurfaceDataPositionView(uvec2 coords, ivec2 depthBufferSize)
{
	//uvec2 depthBufferSize = textureSize(sampler2DArray(zPrePassSurface.container), zPrePassSurface.page);

	vec3 texCoords = vec3(float(coords.x) / float(depthBufferSize.x), float(coords.y)/ float(depthBufferSize.y), zPrePassSurface.page);

	//@NOTE(Serge): if we change our z pre pass to use projection, we should do an unproject here
	//return UnprojectDepthBufferZValue(texture(sampler2DArray(zPrePassSurface.container), texCoords).z);

	return texture(sampler2DArray(zPrePassSurface.container), texCoords).r;
}

// vec3 ComputePositionViewFromZ(vec2 positionScreen, float viewSpaceZ)
// {
// 	vec2 screenSpaceRay = vec2(positionScreen.x / projection[0][0], positionScreen.y / projection[1][1]);
// 	vec3 positionView;
// 	positionView.z = viewSpaceZ;
// 	positionView.xy = screenSpaceRay.xy * positionView.z;
// 	return positionView;
// }