#ifndef GLSL_MATERIAL_SAMPLING
#define GLSL_MATERIAL_SAMPLING

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"

vec4 SampleMaterialDiffuse(in Material m, vec3 uv)
{
	//Material m = GetMaterial(drawID, uv);
	
	Tex2DAddress addr = m.albedo.texture;

	vec3 coord = MakeTextureCoord(addr, uv);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(m.albedo.color.rgb,1) + modifier * SampleTexture(addr, coord, mipmapLevel);
}

vec4 SampleMaterialNormal(mat3 TBN, vec3 normal, in Material m, vec3 uv)
{
//	Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.normalMap.texture;

	vec3 coord = vec3(uv.rg, addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	vec3 normalMapNormal = SampleTexture(addr, coord, mipmapLevel).rgb;

	normalMapNormal = normalize(normalMapNormal * 2.0 - 1.0);

	normalMapNormal = normalize(TBN * normalMapNormal);

	return  (1.0 - modifier) * vec4(normalize(normal), 1) +  modifier * vec4(normalMapNormal, 1);
}

vec4 SampleMaterialEmission(in Material m, vec3 uv)
{
//	Material m = GetMaterial(drawID, uv);
	
	Tex2DAddress addr = m.emission.texture;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(m.emission.color.rgb,1) + modifier * SampleTexture(addr, coord, mipmapLevel);
}

vec4 SampleMaterialMetallic(in Material m, vec3 uv)
{
//	Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.metallic.texture;

	float modifier = GetTextureModifier(addr);

	vec4 color = m.metallic.color;

	return (1.0 - modifier) * color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialRoughness(in Material m, vec3 uv)
{
	//@TODO(Serge): put this back to roughnessTexture1
	//Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.roughness.texture;

	float modifier = GetTextureModifier(addr);

	vec4 color = m.roughness.color;

	//@TODO(Serge): put this back to not using the alpha
	return (1.0 - modifier) * color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialAO(in Material m, vec3 uv)
{
//	Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.ao.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(1.0) + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

float SampleAOSurface(vec2 uv)
{
	ivec3 coord = ivec3(ivec2(uv), ambientOcclusionTemporalDenoiseSurface[0].page);

	return  texelFetch(sampler2DArray(ambientOcclusionTemporalDenoiseSurface[0].container), coord, 0).r;
}

vec4 SampleDetail(in Material m, vec3 uv)
{
	//highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	//Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.detail.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.detail.color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

float SampleClearCoat(in Material m, vec3 uv)
{
	//highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	//Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.clearCoat.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.clearCoat.color.r + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).r;
}

vec3 SampleClearCoatNormal(in Material m, vec3 uv)
{
	Tex2DAddress addr = m.clearCoatNormal.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.clearCoatNormal.color.rgb + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).rgb;
}

float SampleClearCoatRoughness(in Material m, vec3 uv)
{
	//Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.clearCoatRoughness.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.clearCoatRoughness.color.r + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).r;
}

// vec4 SampleMaterialPrefilteredRoughness(vec3 uv, float roughnessValue)
// {
// 	Tex2DAddress addr = skylight.prefilteredRoughnessTexture;
// 	//z is up so we rotate this... not sure if this is right?
// 	return textureLod(samplerCubeArray(addr.container), vec4(uv.r, uv.b, -uv.g, addr.page), roughnessValue);
// }


float SampleMaterialHeight(in Material m, vec3 uv)
{
	//Material m = GetMaterial(drawID, uv);
	//highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = m.height.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.height.color.r + modifier * (1.0 - SampleTexture(addr, vec3(uv.rg, addr.page), 0).r);//texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page)).r);
}

vec3 SampleAnisotropyDirection(mat3 TBN, vec3 tangent, in Material m, vec3 uv)
{
	//Material m = GetMaterial(drawID, uv);

	Tex2DAddress addr = m.anisotropy.texture;

	float modifier = GetTextureModifier(addr);

	vec3 anisotropyDirection = SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).rrr; //@TODO(Serge): fix this to be .rgb - won't work at the moment with our content

	//anisotropyDirection = anisotropyDirection;

	return (1.0 - modifier) * tangent + modifier * TBN * anisotropyDirection;// + modifier * (fs_in.TBN * anisotropyDirection);
}

vec3 ParallaxMapping(in Material m, vec3 uv, vec3 viewDir)
{
	//highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = m.height.texture;

	float modifier = GetTextureModifier(addr);

	const float heightScale = m.heightScale;

	if(modifier <= 0.0 || heightScale <= 0.0)
		return uv;

	float currentLayerDepth = 0.0;

	const float minLayers = 8;
	const float maxLayers = 32;

	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

	float layerDepth = 1.0 / numLayers;

	vec2 P = viewDir.xy / viewDir.z * heightScale;

	vec2 deltaTexCoords = P / numLayers;

	vec2 currentTexCoords = uv.rg;
	float currentDepthMapValue = SampleMaterialHeight(m, uv);

	while(currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;

		currentDepthMapValue = SampleMaterialHeight(m, vec3(currentTexCoords, uv.b));

		currentLayerDepth += layerDepth;
	}

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = SampleMaterialHeight(m, vec3(prevTexCoords, uv.b)) - currentLayerDepth + layerDepth;

	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
	
	return vec3(finalTexCoords, uv.b);
}



#endif