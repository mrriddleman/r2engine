#ifndef GLSL_MATERIAL_DATA
#define GLSL_MATERIAL_DATA

#include "Common/Texture.glsl"

struct RenderMaterialParam
{
	Tex2DAddress texture;
	vec4 color;
};

struct CubemapRenderMaterialParam
{
	CubemapAddress texture;
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

	CubemapRenderMaterialParam cubemap;

	int 	doubleSided;
	float 	heightScale;
	float	reflectance;
	float 	emissionStrength;

	float	alphaCutoff;
	float	unused1;
	float	unused2;
	float	unused3;
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

uint GetMaterialIndex(uint drawID, uint localMaterialIndex)
{
	highp uint matIndex = localMaterialIndex + materialOffsets[drawID].x;
	return matIndex;
}

Material GetMaterial(uint drawID, uint localMaterialIndex)
{
	highp uint materialIndex = GetMaterialIndex(drawID, localMaterialIndex);
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