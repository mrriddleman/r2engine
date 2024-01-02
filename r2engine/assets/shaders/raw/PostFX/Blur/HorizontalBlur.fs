#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

#define HORIZONTAL_WEIGHTS 1

#include "Common/Texture.glsl"
#include "PostFX/Blur/BlurConstants.glsl"

layout(location = 0) out vec4 oBlurColor;

#ifdef GL_NV_gpu_shader5
uniform uint64_t textureContainerToBlur;
#else 
uniform sampler2DArray textureContainerToBlur;
#endif
uniform float texturePage;
uniform float textureLod;

//horizontal offsets
// const ivec2 offsets[7] = {{-3, 0}, {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}};

// const float weights[7] = {0.001f, 0.028f, 0.233f, 0.474f, 0.233f, 0.028f, 0.001f};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

void main()
{
	Tex2DAddress addr;
	addr.container = textureContainerToBlur;
	addr.page = texturePage;
	addr.channel = 0;

	vec4 color = vec4(0);
	for(uint i = 0; i < 7; ++i)
	{
		color += SampleTextureLodOffsetRGBA(addr, fs_in.texCoords.xy, offsets[i], textureLod) * weights[i];
	}

	oBlurColor = vec4(color.rgb, 1.0);
}