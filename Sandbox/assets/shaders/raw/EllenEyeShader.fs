#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"
// struct Tex2DAddress
// {
// 	uint64_t container;
// 	float page;	
// 	int channel;
// };

// struct RenderMaterialParam
// {
// 	Tex2DAddress texture;
// 	vec4 color;
// };

// struct Material
// {
// 	RenderMaterialParam albedo;
// 	RenderMaterialParam normalMap;
// 	RenderMaterialParam emission;
// 	RenderMaterialParam metallic;
// 	RenderMaterialParam roughness;
// 	RenderMaterialParam ao;
// 	RenderMaterialParam height;
// 	RenderMaterialParam anisotropy;
// 	RenderMaterialParam detail;

// 	RenderMaterialParam clearCoat;
// 	RenderMaterialParam clearCoatRoughness;
// 	RenderMaterialParam clearCoatNormal;

// 	int 	doubleSided;
// 	float 	heightScale;
// 	float	reflectance;
// 	int 	padding;
// };

// layout (std430, binding = 1) buffer Materials
// {
// 	Material materials[];
// };

// layout (std430, binding = 7) buffer MaterialOffsets
// {
// 	uint materialOffsets[];
// };

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	vec3 fragPos;
	mat3 TBN;

	flat uint drawID;
} fs_in;

// float GetTextureModifier(Tex2DAddress addr)
// {
// 	return float( min(max(addr.container, 0), 1) );
// }

// vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
// {
// 	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
// 	Tex2DAddress addr = materials[texIndex].albedo.texture;

// 	vec3 coord = vec3(uv.rg,addr.page);

// 	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

// 	float modifier = GetTextureModifier(addr);

// 	return (1.0 - modifier) * vec4(materials[texIndex].albedo.color.rgb, 1) + modifier * textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
// }

void main()
{
	Material m = GetMaterial(fs_in.drawID, fs_in.texCoords);
	vec4 diffuseColor = SampleMaterialDiffuse(m, fs_in.texCoords);
	FragColor = vec4(1.0, 1.0, 1.0, 1.0) * diffuseColor;
}
