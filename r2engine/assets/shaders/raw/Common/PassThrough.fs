#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/UniformBuffers/Surfaces.glsl"

uniform uint64_t inputTextureContainer;
uniform float inputTexturePage;
uniform float inputTextureLod;

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

vec3 SampleTexture(uint64_t textureContainer, float texturePage, vec2 texCoords);

void main()
{
	FragColor = vec4(SampleTexture(inputTextureContainer, inputTexturePage, fs_in.texCoords.xy), 1.0);
}

vec3 SampleTexture(uint64_t textureContainer, float texturePage, vec2 texCoords)
{
	vec3 coord = vec3(texCoords.x, texCoords.y, texturePage);
	return textureLod(sampler2DArray(textureContainer), coord, inputTextureLod).rgb;
}