#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout(location = 0) out vec4 oBlurColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

uniform Tex2DAddress textureToBlur;

//vertical offsets
const ivec2 offsets[7] = {{0, -3}, {0, -2}, {0, -1}, {0, 0}, {0, 1}, {0, 2}, {0, 3}};

const float weights[7] = {0.001f, 0.028f, 0.233f, 0.474f, 0.233f, 0.028f, 0.001f};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

void main()
{
	vec3 uvPage = vec3(fs_in.texCoords.xy, textureToBlur.page);

	vec4 color = vec4(0);
	for(uint i = 0; i < 7; ++i)
	{
		color += textureOffset(sampler2DArray(textureToBlur.container), uvPage, offsets[i]) * weights[i];
	}

	oBlurColor = vec4(color.rgb, 1.0);
}