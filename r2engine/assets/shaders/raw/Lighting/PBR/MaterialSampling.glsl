#ifndef GLSL_MATERIAL_SAMPLING
#define GLSL_MATERIAL_SAMPLING

#include "Input/ShaderBufferObjects/MaterialData.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"

vec4 SampleCubemapMaterialDiffuse(in Material m, vec3 uv)
{
	return SampleCubemapRGBA(m.cubemap.texture, uv);
}

vec4 SampleMaterialDiffuse(in Material m, vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.albedo.texture;

	return m.albedo.color * SampleMaterialTexture(addr, uv, vec4(1));

	// vec3 coord = MakeTextureCoord(addr, uv);

	// float mipmapLevel = TextureQueryLod(addr, uv.rg);

	// float modifier = GetTextureModifier(addr);

	// return (1.0 - modifier) * m.albedo.color + modifier * SampleTexture(addr, coord, mipmapLevel);
}

vec4 SampleMaterialNormal(mat3 TBN, vec3 normal, in Material m, vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.normalMap.texture;
	float modifier = GetTextureModifier(addr);

	vec3 normalToReturn = normalize(normal);

	if(modifier > 0.0f)
	{
		normalToReturn = SampleTextureAddr(addr, uv).rgb;
		normalToReturn = normalize(normalToReturn * 2.0 - 1.0);
		normalToReturn = normalize(TBN * normalToReturn);
	}
	return vec4(normalToReturn, 1);
	// vec3 coord = vec3(uv.rg, addr.page);

	// float mipmapLevel = TextureQueryLod(addr, uv.rg);

	// float modifier = ;

	// vec3 normalMapNormal = SampleTexture(addr, coord, mipmapLevel).rgb;

	

//	return  (1.0 - modifier) * vec4(normalize(normal), 1) +  modifier * vec4(normalMapNormal, 1);
}

vec4 SampleMaterialEmission(in Material m, vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.emission.texture;

	return m.emission.color * SampleMaterialTexture(addr, uv, vec4(1));
	// vec3 coord = vec3(uv.rg,addr.page);

	// float mipmapLevel = TextureQueryLod(addr, uv.rg);

	// float modifier = GetTextureModifier(addr);

	// return (1.0 - modifier) * vec4(m.emission.color.rgb,1) + modifier * SampleTexture(addr, coord, mipmapLevel);
}

vec4 SampleMaterialMetallic(in Material m,  vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.metallic.texture;

	return m.metallic.color * SampleMaterialTexture(addr, uv, vec4(1));
	// float modifier = GetTextureModifier(addr);

	// vec4 color = m.metallic.color;

	// return (1.0 - modifier) * color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialRoughness(in Material m, vec2 uv[NUM_TEX_COORDS])
{
	//@TODO(Serge): put this back to roughnessTexture1
	Tex2DAddress addr = m.roughness.texture;

	return m.roughness.color * SampleMaterialTexture(addr, uv, vec4(1));
	// float modifier = GetTextureModifier(addr);

	// vec4 color = m.roughness.color;

	// //@TODO(Serge): put this back to not using the alpha
	// return (1.0 - modifier) * color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialAO(in Material m, vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.ao.texture;

	//@TODO(Serge): this is wrong atm - fix with data - should be: m.ao.color * SampleMaterialTexture(addr, uv)
	return vec4(1) * SampleMaterialTexture(addr, uv, vec4(1));
	// float modifier = GetTextureModifier(addr);

	// return (1.0 - modifier) * vec4(1.0) + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

float SampleAOSurface(vec2 uv)
{
	return TexelFetch(ambientOcclusionTemporalDenoiseSurface[0], ivec2(uv), 0).r;
}

//@NOTE(Serge): old code - probably not used
// vec4 SampleDetail(in Material m, vec3 uv)
// {
// 	//highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
// 	Tex2DAddress addr = m.detail.texture;

// 	float modifier = GetTextureModifier(addr);

// 	return (1.0 - modifier) * m.detail.color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
// }

float SampleClearCoat(in Material m, vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.clearCoat.texture;

	return m.clearCoat.color.r * SampleMaterialTexture(addr, uv, vec4(1)).r;
	// float modifier = GetTextureModifier(addr);

	// return (1.0 - modifier) * m.clearCoat.color.r + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).r;
}

vec3 SampleClearCoatNormal(in Material m, vec3 uv)
{
	Tex2DAddress addr = m.clearCoatNormal.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.clearCoatNormal.color.rgb + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).rgb;
}

float SampleClearCoatRoughness(in Material m, vec2 uv[NUM_TEX_COORDS])
{
	Tex2DAddress addr = m.clearCoatRoughness.texture;

	return m.clearCoatRoughness.color.r * SampleMaterialTexture(addr, uv, vec4(1)).r;
	// float modifier = GetTextureModifier(addr);

	// return (1.0 - modifier) * m.clearCoatRoughness.color.r + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).r;
}

float SampleMaterialHeight(in Material m, vec3 uv)
{
	Tex2DAddress addr = m.height.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * m.height.color.r + modifier * (1.0 - SampleTexture(addr, vec3(uv.rg, addr.page), 0).r);
}

vec3 SampleAnisotropyDirection(mat3 TBN, vec3 tangent, in Material m, vec3 uv)
{
	Tex2DAddress addr = m.anisotropy.texture;

	float modifier = GetTextureModifier(addr);

	vec3 anisotropyDirection = SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).rrr; //@TODO(Serge): fix this to be .rgb - won't work at the moment with our content

	return (1.0 - modifier) * tangent + modifier * TBN * anisotropyDirection;// + modifier * (fs_in.TBN * anisotropyDirection);
}

vec3 ParallaxMapping(in Material m, vec3 uv, vec3 viewDir)
{
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