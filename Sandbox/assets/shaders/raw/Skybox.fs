#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"

in VS_OUT
{
	vec3 texCoords;
	flat uint drawID;
} fs_in;

void main()
{
	Material m = GetCubemapMaterial(fs_in.drawID);

	vec4 sampledCubeColor = SampleCubemapMaterialDiffuse(m, fs_in.texCoords);

	FragColor = vec4(sampledCubeColor.rgb, 1.0);
}
