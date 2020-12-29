#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;

layout (location = 0) out vec4 FragColor;

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

	vec4 baseColor;
	float specular;
	float roughness;
	float metallic;
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	FragColor = vec4(sampledColor.r, sampledColor.g, sampledColor.b, 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	//float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	return texture(sampler2DArray(addr.container), coord);
}