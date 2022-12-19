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

vec3 SampleTexture(Tex2DAddress tex, vec2 texCoords, ivec2 offset)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return textureOffset(sampler2DArray(tex.container), coord, offset).rgb;
}

vec3 SampleTexture(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return texture(sampler2DArray(tex.container), coord).rgb;
}

vec4 SampleTextureRGBA(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return texture(sampler2DArray(tex.container), coord);
}

vec3 SampleTextureLodZero(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return textureLod(sampler2DArray(tex.container), coord, 0.0).rgb;
}

vec4 SampleTextureLodZeroRGBA(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return textureLod(sampler2DArray(tex.container), coord, 0.0);
}

vec3 SampleTextureLodZeroOffset(Tex2DAddress tex, vec2 texCoords, ivec2 offset)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return textureLodOffset(sampler2DArray(tex.container), coord, 0.0, offset).rgb;
}

#endif