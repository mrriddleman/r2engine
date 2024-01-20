#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Common/Texture.glsl"

uniform uint64_t inputTextureContainer;
uniform float inputTexturePage;
uniform float inputTextureLod;

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

void main()
{
	Tex2DAddress addr;
	addr.container = inputTextureContainer;
	addr.page = inputTexturePage;
	int lod = int(inputTextureLod);
	addr.channel = uint(lod);

	vec4 sample0 = SampleMSTexel(addr, ivec2(gl_FragCoord.xy), lod);
	vec4 sample1 = SampleMSTexel(addr, ivec2(gl_FragCoord.xy), lod + 1);

	FragColor = normalize(mix(sample0, sample1, 0.5));
}
