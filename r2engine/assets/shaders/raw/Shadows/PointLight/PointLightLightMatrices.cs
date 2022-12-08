#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Common/CommonFunctions.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"

layout (local_size_x = NUM_SIDES_FOR_POINTLIGHT, local_size_y = 1, local_size_z = 1) in;

struct LookAtVectors
{
	vec3 dir;
	vec3 up;
};

const LookAtVectors lookAtVectors[NUM_SIDES_FOR_POINTLIGHT] =
{
	{vec3(1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)},
	{vec3(-1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)},
	{vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)},
	{vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, -1.0)},
	{vec3(0.0, 0.0, 1.0), vec3(0.0, -1.0, 0.0)},
	{vec3(0.0, 0.0, -1.0), vec3(0.0, -1.0, 0.0)},
};

void main(void)
{
	int pointLightIndex = (int)shadowCastingPointLights[int(gl_WorkGroupID.x)];
	int side = int(gl_LocalInvocationID.x);

	mat4 lightView = LookAt(pointLights[pointLightIndex].position.xyz, pointLights[pointLightIndex].position.xyz + lookAtVectors[side].dir, lookAtVectors[side].up);

	//pointLights[pointLightIndex].lightProperties.intensity = 50;
	mat4 lightProj = Projection(PI/2.0, 1, exposureNearFar.y, pointLights[pointLightIndex].lightProperties.intensity);

	pointLights[pointLightIndex].lightSpaceMatrices[side] = lightProj * lightView;
}