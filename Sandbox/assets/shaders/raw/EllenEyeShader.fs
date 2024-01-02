#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	vec3 fragPos;
	mat3 TBN;

	flat uint drawID;
} fs_in;

void main()
{
	Material m = GetMaterial(fs_in.drawID, fs_in.texCoords);
	vec4 diffuseColor = SampleMaterialDiffuse(m, fs_in.texCoords);
	FragColor = vec4(1.0, 1.0, 1.0, 1.0) * diffuseColor;
}
