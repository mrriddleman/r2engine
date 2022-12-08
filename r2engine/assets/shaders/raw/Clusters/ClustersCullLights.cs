#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 16, local_size_y = 16, local_size_z = 2) in;

#include "Common/Defines.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/ClusterData.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"

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
