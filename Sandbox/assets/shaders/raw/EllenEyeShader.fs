#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

const uint NUM_TEXTURES_PER_DRAWID = 8;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct Material
{
	Tex2DAddress diffuseTexture1;
	Tex2DAddress specularTexture1;
	Tex2DAddress normalMapTexture1;
	Tex2DAddress emissionTexture1;
	Tex2DAddress metallicTexture1;
	Tex2DAddress roughnessTexture1;
	Tex2DAddress aoTexture1;
	Tex2DAddress heightTexture1;

	vec3 baseColor;
	float specular;
	float roughness;
	float metallic;
	float reflectance;
	float ambientOcclusion;
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	vec3 fragPos;
	mat3 TBN;

	flat uint drawID;
} fs_in;

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].baseColor,1) + modifier * textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
}

void main()
{
	FragColor = vec4(1.0, 1.0, 1.0, 1.0) * SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

}
