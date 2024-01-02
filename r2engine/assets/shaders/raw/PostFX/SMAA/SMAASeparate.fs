#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor0;
layout (location = 1) out vec4 FragColor1;

#include "Common/Texture.glsl"

#ifdef GL_NV_gpu_shader5
uniform uint64_t inputTextureContainer;
#else
uniform sampler2DArray inputTextureContainer;
#endif
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
	addr.channel = int(inputTextureLod);

	FragColor0 = SampleMSTexel(addr, ivec2(gl_FragCoord.xy), addr.channel);
	FragColor1 = SampleMSTexel(addr, ivec2(gl_FragCoord.xy), addr.channel + 1);
}