#ifndef GLSL_MATERIAL_DATA
#define GLSL_MATERIAL_DATA

#include "Common/Texture.glsl"

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

uint GetMaterialIndex(uint drawID, vec3 uv)
{
	highp uint matIndex = uint(round(uv.z)) + materialOffsets[drawID];
	return matIndex;
}

Material GetMaterial(uint drawID, vec3 uv)
{
	highp uint materialIndex = GetMaterialIndex(drawID, uv);
	return materials[materialIndex];
}

Material GetCubemapMaterial(uint drawID)
{
	highp uint texIndex =  materialOffsets[drawID];
	return materials[texIndex];
}

#endif