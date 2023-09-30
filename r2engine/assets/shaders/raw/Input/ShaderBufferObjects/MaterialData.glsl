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
	uvec4 materialOffsets[]; 
	//x holds the actual material offset
	//in editor builds - y is the entity ID
	//in editor builds - z is the instance of the entity ID (-1 for the first one)
};

uint GetMaterialIndex(uint drawID, vec3 uv)
{
	highp uint matIndex = uint(round(uv.z)) + materialOffsets[drawID].x;
	return matIndex;
}

Material GetMaterial(uint drawID, vec3 uv)
{
	highp uint materialIndex = GetMaterialIndex(drawID, uv);
	return materials[materialIndex];
}

Material GetCubemapMaterial(uint drawID)
{
	highp uint texIndex =  materialOffsets[drawID].x;
	return materials[texIndex];
}

uvec2 GetEntityInstance(uint drawID)
{
	return materialOffsets[drawID].yz;
}

#endif