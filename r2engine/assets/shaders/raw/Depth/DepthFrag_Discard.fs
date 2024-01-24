#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"

in VS_OUT
{
	flat uint drawID;
	flat uint materialIndex;
	vec3 texCoords; 
} fs_in;

void main()
{
	Material m = GetMaterial(fs_in.drawID, fs_in.materialIndex);

	//@TODO(Serge): add in the texCoords1
	vec2 uv[2];
	uv[0] = fs_in.texCoords.rg;
	uv[1] = vec2(0);
	vec4 sampledColor = SampleMaterialDiffuse(m, uv);

	if(sampledColor.a < m.alphaCutoff)
		discard;
}