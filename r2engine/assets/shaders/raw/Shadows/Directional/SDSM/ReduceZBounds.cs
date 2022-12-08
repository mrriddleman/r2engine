#version 450 core
#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Common/Texture.glsl"
#include "Depth/DepthUtils.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/SDSMParams.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"

#define REDUCE_ZBOUNDS_BLOCK_DIM 16
#define REDUCE_ZBOUNDS_BLOCK_SIZE (REDUCE_ZBOUNDS_BLOCK_DIM * REDUCE_ZBOUNDS_BLOCK_DIM)

layout (local_size_x = REDUCE_ZBOUNDS_BLOCK_DIM, local_size_y = REDUCE_ZBOUNDS_BLOCK_DIM, local_size_z = 1) in;

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

float ComputeSurfaceDataPositionView(uvec2 coords, ivec2 depthBufferSize)
{
	vec3 texCoords = vec3(float(coords.x) / float(depthBufferSize.x), float(coords.y)/ float(depthBufferSize.y), zPrePassShadowsSurface[0].page);

	return LinearizeDepth(texture(sampler2DArray(zPrePassShadowsSurface[0].container), texCoords).r, exposureNearFar.y, exposureNearFar.z);
}

