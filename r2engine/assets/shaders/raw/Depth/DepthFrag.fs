#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"

in VS_OUT
{
	flat uint drawID;
	vec3 texCoords; 
} fs_in;

void main()
{
	#ifdef DISCARD_ALPHA
	Material m = GetMaterial(fs_in.drawID, fs_in.texCoords);

	vec4 sampledColor = SampleMaterialDiffuse(m, fs_in.texCoords);

	if(sampledColor.a < 0.5)
		discard;
	#endif
}