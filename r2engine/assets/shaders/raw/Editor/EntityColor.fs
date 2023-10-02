#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out uvec2 FragColor;

#include "Input/ShaderBufferObjects/MaterialData.glsl"

in VS_OUT
{
	flat uint drawID;
	vec3 texCoords; 
} fs_in;

void main()
{
	FragColor = GetEntityInstance(fs_in.drawID);
}