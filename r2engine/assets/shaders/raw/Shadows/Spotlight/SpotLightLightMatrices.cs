#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Common/CommonFunctions.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void main(void)
{
	int spotLightIndex = (int)shadowCastingSpotLights[int(gl_WorkGroupID.x)];

	SpotLight spotLight = spotLights[spotLightIndex];

	mat4 lightView = LookAt(spotLight.position.xyz, spotLight.position.xyz + spotLight.direction.xyz, GLOBAL_UP);
	
	//@NOTE(Serge): the 1 here is the aspect ratio - since we're using a size of shadowMapSizes.x x shadowMapSizes.x it will be 1. If this changes, we need to change this as well
	mat4 lightProj = Projection(acos(spotLight.direction.w), 1, exposureNearFar.y, spotLight.lightProperties.intensity);

	spotLights[spotLightIndex].lightSpaceMatrix = lightProj * lightView;
}