#ifndef GLSL_TEXTURE
#define GLSL_TEXTURE

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

vec4 SampleTexture(Tex2DAddress addr, vec3 coord, float mipmapLevel)
{
	vec4 textureSample = textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
	return addr.channel < 0 ? vec4(textureSample.rgba) : vec4(textureSample[addr.channel]); //no rgb right now
}

vec3 MakeTextureCoord(Tex2DAddress addr, vec3 uv)
{
	return vec3(uv.rg, addr.page);
}

#endif