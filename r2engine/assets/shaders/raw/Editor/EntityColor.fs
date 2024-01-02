#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
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
	uvec2 entityInstance = GetEntityInstance(fs_in.drawID);

	FragColor = entityInstance;//vec4(clamp(float(entityInstance.x), 0.0f, 1.0f), 0.0f, 0.0f, 1.0f);//entityInstance.x;

}