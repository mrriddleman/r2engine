#ifndef GLSL_TEXTURE
#define GLSL_TEXTURE

#include "Common/Defines.glsl"

struct Tex2DAddress
{
#ifdef GL_NV_gpu_shader5
	uint64_t  container;
#else
	sampler2DArray container;
#endif
	float page;
	uint channel;
};

struct CubemapAddress
{
#ifdef GL_NV_gpu_shader5
	uint64_t  container;
#else
	samplerCubeArray container;
#endif
	float page;
	uint channel;
};

uint GetChannel(Tex2DAddress addr)
{
	//the channel is in the lower 16 bits of the addr.channel
	return (addr.channel & uint(0x0000ffff));
}

uint GetTexCoordIndex(Tex2DAddress addr)
{
	//the texture coordinate is in the high 16 bits
	return (addr.channel & uint(0xffff0000)) >> 16;
}

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

vec2 ResolveUVArray(Tex2DAddress addr, vec2 texCoords[NUM_TEX_COORDS])
{
	return texCoords[GetTexCoordIndex(addr)];
}

vec3 MakeTextureCoord(Tex2DAddress addr, vec2 texCoords[NUM_TEX_COORDS])
{
	vec2 uv = ResolveUVArray(addr, texCoords);
	return vec3(uv, addr.page);
}

vec3 MakeTextureCoord(Tex2DAddress addr, vec3 uv)
{
	return vec3(uv.rg, addr.page);
}

ivec3 MakeTextureCoord(Tex2DAddress addr, ivec2 texCoords)
{
	return ivec3(texCoords, (int)addr.page);
}

float TextureQueryLod(Tex2DAddress tex, vec2 uv)
{
#ifdef GL_NV_gpu_shader5
	return textureQueryLod(sampler2DArray(tex.container), uv).x;
#else
	return textureQueryLod(tex.container), uv).x;
#endif
}

vec4 SampleTextureAddr(Tex2DAddress addr, vec2 texCoords[NUM_TEX_COORDS])
{
	vec3 coord = MakeTextureCoord(addr, texCoords);

	float mipmapLevel = TextureQueryLod(addr, coord.rg);

#ifdef GL_NV_gpu_shader5
	vec4 textureSample = textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
#else
	vec4 textureSample = textureLod(addr.container, coord, mipmapLevel);
#endif

	uint channel = GetChannel(addr);

	return channel > TEX_CHANNEL_A ? vec4(textureSample) : vec4(textureSample[channel]);
}

vec4 SampleMaterialTexture(Tex2DAddress addr, vec2 texCoords[NUM_TEX_COORDS], vec4 defaultColor)
{
	vec4 textureColor = defaultColor;
	
	float modifier = GetTextureModifier(addr);

	if(modifier > 0.0f)
	{
		textureColor = SampleTextureAddr(addr, texCoords);
	}

	return textureColor;
}

vec4 SampleTexture(Tex2DAddress addr, vec3 coord, float mipmapLevel)
{
#ifdef GL_NV_gpu_shader5
	vec4 textureSample = textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
#else
	vec4 textureSample = textureLod(addr.container, coord, mipmapLevel);
#endif

	uint channel = GetChannel(addr);

	return channel > TEX_CHANNEL_A ? vec4(textureSample.rgba) : vec4(textureSample[channel]); //no rgb right now
}

vec3 SampleTexture(Tex2DAddress tex, vec2 texCoords, ivec2 offset)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return textureOffset(sampler2DArray(tex.container), coord, offset).rgb;
#else
	return textureOffset(tex.container, coord, offset).rgb;
#endif
}

vec3 SampleTexture(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return texture(sampler2DArray(tex.container), coord).rgb;
#else
	return texture(tex.container, coord).rgb;
#endif
}

float SampleTextureR(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return texture(sampler2DArray(tex.container), coord).r;
#else
	return texture(tex.container, coord).r;
#endif
}

vec4 SampleTextureRGBA(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return texture(sampler2DArray(tex.container), coord);
#else
	return texture(tex.container, coord);
#endif
}


vec3 SampleTextureLodZero(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
	return textureLod(sampler2DArray(tex.container), coord, 0.0).rgb;
}

vec4 SampleTextureLodZeroRGBA(Tex2DAddress tex, vec2 texCoords)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return textureLod(sampler2DArray(tex.container), coord, 0.0);
#else
	return textureLod(tex.container, coord, 0.0);
#endif
} 

vec4 SampleTextureLodRGBA(Tex2DAddress tex, vec2 uv, float lod)
{
	vec3 coord = MakeTextureCoord(tex, vec3(uv, 0));
#ifdef GL_NV_gpu_shader5
	return textureLod(sampler2DArray(tex.container), coord, lod);
#else
	return textureLod(tex.container, coord, lod);
#endif
}

vec3 SampleTextureLodZeroOffset(Tex2DAddress tex, vec2 texCoords, ivec2 offset)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return textureLodOffset(sampler2DArray(tex.container), coord, 0.0, offset).rgb;
#else
	return textureLodOffset(tex.container, coord, 0.0, offset).rgb;
#endif
}

vec4 SampleTextureLodOffsetRGBA(Tex2DAddress tex, vec2 texCoords, ivec2 offset, float lod)
{
	vec3 coord = MakeTextureCoord(tex, vec3(texCoords, 0));
#ifdef GL_NV_gpu_shader5
	return textureLodOffset(sampler2DArray(tex.container), coord, lod, offset);
#else
	return textureLodOffset(tex.container, coord, lod, offset);
#endif
}

vec4 SampleMSTexel(Tex2DAddress tex, ivec2 texCoords, int sampleCoord)
{
	ivec3 coord = MakeTextureCoord(tex, texCoords);
#ifdef GL_NV_gpu_shader5
	return texelFetch(sampler2DMSArray(tex.container), coord, sampleCoord);
#else
	return texelFetch(tex.container, coord, sampleCoord);
#endif
}

vec4 TexelFetch(Tex2DAddress tex, ivec2 texCoords, int lod)
{
	ivec3 coord = MakeTextureCoord(tex, texCoords);
#ifdef GL_NV_gpu_shader5
	return texelFetch(sampler2DArray(tex.container), coord, lod);
#else
	return texelFetch(tex.container, coord, lod);
#endif
}

vec4 SampleCubemapRGBA(CubemapAddress tex, vec3 texCoords)
{
#ifdef GL_NV_gpu_shader5
	return texture(samplerCubeArray(tex.container), vec4(texCoords, tex.page));
#else
	return texture(tex.container, vec4(texCoords, tex.page));
#endif
}

vec4 SampleCubemapPageRGBA(CubemapAddress tex, vec3 texCoords, float page)
{
#ifdef GL_NV_gpu_shader5
	return texture(samplerCubeArray(tex.container), vec4(texCoords, page));
#else
	return texture(tex.container, vec4(texCoords, page));
#endif
}

vec4 SampleCubemapLodRGBA(CubemapAddress tex, vec3 texCoords, float lod)
{
#ifdef GL_NV_gpu_shader5
	return textureLod(samplerCubeArray(tex.container), vec4(texCoords, tex.page), lod);
#else
	return textureLod(tex.container, vec4(texCoords, tex.page), lod);
#endif
}



vec2 TexelFetchLodOffset(Tex2DAddress inputTexture, ivec2 uv, ivec2 uvOffset, int lod)
{
	return TexelFetch(inputTexture, uv + uvOffset, lod).rg;
}

vec4 GatherOffset(Tex2DAddress tex, vec2 uv, ivec2 offset)
{
	vec3 texCoord = MakeTextureCoord(tex, vec3(uv, 0));
#ifdef GL_NV_gpu_shader5
	return textureGatherOffset(sampler2DArray(tex.container), texCoord, offset);
#else
	return textureGatherOffset(tex.container, texCoord, offset);
#endif
}

vec2 TextureSize(Tex2DAddress tex, int lod)
{
#ifdef GL_NV_gpu_shader5
	return vec2(textureSize(sampler2DArray(tex.container), lod));
#else
	return vec2(textureSize(tex.container, lod));
#endif
}

vec4 SampleTextureIRGBA(Tex2DAddress tex, vec2 uv)
{
	ivec3 texCoord = ivec3(uv.r, uv.g, tex.page);
#ifdef GL_NV_gpu_shader5
	return texture(sampler2DArray(tex.container), texCoord);
#else
	return texture(tex.container, texCoord);
#endif
}

vec4 SampleTextureWithPage(Tex2DAddress tex, vec2 uv, float page)
{
	vec3 coord = vec3(uv, page);
#ifdef GL_NV_gpu_shader5
	return texture(sampler2DArray(tex.container), coord);
#else
	return texture(tex.container, coord);
#endif
}

#endif