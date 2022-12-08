#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/ShaderBufferObjects/MaterialData.glsl"

in VS_OUT
{
	vec3 texCoords; 
	flat uint drawID;

} fs_in;

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);

void main()
{
	FragColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	return vec4(materials[texIndex].albedo.color.rgb, 1);
}