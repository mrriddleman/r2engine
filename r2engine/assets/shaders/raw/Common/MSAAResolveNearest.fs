#version 450 core

#extension GL_NV_gpu_shader5 : enable

//layout (location = 0) out vec4 FragColor;

#include "Common/Texture.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Common/CommonFunctions.glsl"

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
	addr.channel = int(inputTextureLod);

	float result = 1.0f; //for reverse-z set to 0.0f

	//for(int i = 0; i < 2; ++i)
	{
		result = Saturate(mix(SampleMSTexel(addr, ivec2(gl_FragCoord.xy), addr.channel).r, SampleMSTexel(addr, ivec2(gl_FragCoord.xy), addr.channel + 1).r, 0.5));
	}

	gl_FragDepth = result;
}