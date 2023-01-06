#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/ShaderBufferObjects/MaterialData.glsl"

in VS_OUT
{
	vec3 texCoords; 
	flat uint drawID;

} fs_in;

void main()
{
	Material m = GetMaterial(fs_in.drawID, fs_in.texCoords);

	FragColor = vec4(m.albedo.color.rgb, 1);
}
