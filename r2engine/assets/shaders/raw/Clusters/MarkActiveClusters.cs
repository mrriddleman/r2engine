#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (local_size_x = 20, local_size_y = 20) in;

#include "Common/Defines.glsl"
#include "Common/Texture.glsl"
#include "Depth/DepthUtils.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Input/ShaderBufferObjects/ClusterData.glsl"

void MarkActiveCluster(uvec2 screenCoord);

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
	uint clusterID = GetClusterIndex(vec2(screenCoord), fovAspectResXResY.zw, exposureNearFar.yz, clusterScaleBias.xy, clusterTileSizes, zPrePassSurface);

	activeClusters[clusterID] = true;
}
