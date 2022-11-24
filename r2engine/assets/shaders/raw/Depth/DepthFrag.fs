#version 450 core

#extension GL_NV_gpu_shader5 : enable

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

struct RenderMaterialParam
{
	Tex2DAddress texture;
	vec4 color;
};

struct Material
{
	RenderMaterialParam albedo;
	RenderMaterialParam normalMap;
	RenderMaterialParam emission;
	RenderMaterialParam metallic;
	RenderMaterialParam roughness;
	RenderMaterialParam ao;
	RenderMaterialParam height;
	RenderMaterialParam anisotropy;
	RenderMaterialParam detail;

	RenderMaterialParam clearCoat;
	RenderMaterialParam clearCoatRoughness;
	RenderMaterialParam clearCoatNormal;

	int 	doubleSided;
	float 	heightScale;
	float	reflectance;
	int 	padding;
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

layout (std430, binding = 7) buffer MaterialOffsets
{
	uint materialOffsets[];
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

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].albedo.texture;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].albedo.color.rgb,1) + modifier * SampleTexture(addr, coord, mipmapLevel);
}

in VS_OUT
{
	flat uint drawID;
	vec3 texCoords; 
} fs_in;

void main()
{
	#ifdef DISCARD_ALPHA
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	if(sampledColor.a < 0.5)
		discard;
	#endif
}