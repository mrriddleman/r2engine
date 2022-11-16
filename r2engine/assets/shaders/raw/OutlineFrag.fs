#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

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

in VS_OUT
{
	vec3 texCoords; 
	flat uint drawID;

} fs_in;

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);

void main()
{
	FragColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	return vec4(materials[texIndex].albedo.color.rgb, 1);
}