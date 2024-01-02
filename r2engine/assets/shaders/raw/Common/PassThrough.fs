#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/UniformBuffers/Surfaces.glsl"

//@TODO(Serge): clean this up - don't want to use uint64_t if we don't have to
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
	addr.channel = 0;

	FragColor = SampleTextureLodRGBA(addr, fs_in.texCoords.xy, inputTextureLod);
}
