#ifndef GLSL_MATERIAL_INPUT
#define GLSL_MATERIAL_INPUT

#include "Common/Defines.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"
#include "Lighting/PBR/PBR.glsl"

void DefaultBRDFInput(
vec3 baseColor,
vec3 diffuseColor,
vec3 worldFragPos,
vec3 materialNormal,
vec3 worldNormal,
vec3 viewVector,
vec3 uv,
vec3 anisotropyDirection,
vec3 clearCoatNormal,
vec3 multibounceAO,
float reflectance,
float metallic,
float ao,
float perceptualRoughness,
float anisotropy,
float clearCoat,
float clearCoatPerceptualRoughness,
vec3 emission,
inout PixelData pixel)
{
	pixel.worldFragPos = worldFragPos;
	pixel.baseColor = baseColor;
	pixel.N = materialNormal;
	pixel.V = viewVector;
	pixel.uv = uv;
	pixel.diffuseColor = diffuseColor;
	pixel.F0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
	pixel.F0 = CalculateClearCoatBaseF0(pixel.F0, clearCoat);
	pixel.Fd = diffuseColor * Fd_Lambert();
	pixel.multibounceAO = multibounceAO;
	pixel.reflectance = reflectance;
	pixel.metallic = metallic;
	pixel.ao = ao;
	pixel.roughness = perceptualRoughness * perceptualRoughness;
	pixel.NoV = max(dot(pixel.N, pixel.V), 0.0);
	
	//energy compensation is calculated elsewhere in IBL.glsl

	float a2 = pixel.roughness * pixel.roughness;
	pixel.ggxVTerm = sqrt((pixel.NoV - a2 * pixel.NoV) * pixel.NoV + a2);

	pixel.anisotropy = anisotropy;
	pixel.at = max(pixel.roughness * (1.0 + anisotropy), MIN_PERCEPTUAL_ROUGHNESS);
	pixel.ab = max(pixel.roughness * (1.0 - anisotropy), MIN_PERCEPTUAL_ROUGHNESS);
	pixel.anisotropicT = anisotropyDirection;
	pixel.anisotropicB = normalize(cross(worldNormal, anisotropyDirection));
	pixel.ToV = dot(pixel.anisotropicT, pixel.V);
	pixel.BoV = dot(pixel.anisotropicB, pixel.V);

	if(abs(anisotropy) < 1e-5)
	{
		/*
		float ToV = dot(fs_in.tangent, V);
		float BoV = dot(fs_in.bitangent, V);
		*/
		pixel.ggxVTerm = length(vec3(pixel.at * pixel.ToV, pixel.ab * pixel.BoV, pixel.NoV));
	}

	pixel.R = GetReflectionVector(anisotropy, pixel.anisotropicT, pixel.anisotropicB, perceptualRoughness, pixel.V, pixel.N);

	pixel.clearCoat = clearCoat;
	pixel.clearCoatNormal = worldNormal;
	pixel.clearCoatRoughness = CalculateClearCoatRoughness(clearCoatPerceptualRoughness);
	pixel.clearCoatPerceptualRoughness = clearCoatPerceptualRoughness;
	pixel.clearCoatNoV = max(dot(pixel.clearCoatNormal, pixel.V), 0.0);

	pixel.emission = emission;

}

void DefaultWorldMaterialFunction(
	vec3 fragPos,
	uint drawID,
	vec3 uv,
	mat3 TBN,
	vec3 tangent,
	vec3 normal,
	inout PixelData pixelData)
{
	vec3 viewVector = normalize(cameraPosTimeW.xyz - fragPos);

	Material material = GetMaterial(drawID, uv);

	vec3 baseColor = SampleMaterialDiffuse(material, uv).rgb;

	vec3 materialNormal = SampleMaterialNormal(TBN, normal, material, uv).xyz;

	float reflectance = material.reflectance;
	
	float anisotropy = material.anisotropy.color.r;
	
	float metallic = SampleMaterialMetallic(material, uv).r;

	float ao = min(SampleMaterialAO(material, uv).r, SampleAOSurface(gl_FragCoord.xy));

	float perceptualRoughness = clamp(SampleMaterialRoughness(material, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	vec3 multibounceAO = GTAOMultiBounce(ao, diffuseColor);

	vec3 anisotropyDirection = normalize(SampleAnisotropyDirection(TBN, tangent, material, uv));

	//@TODO(Serge): add in the sampling of clear coat materials
	float clearCoat = material.clearCoat.color.r;

	vec3 clearCoatNormal = normal;

	float clearCoatPerceptualRoughness = clamp(material.clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;

	DefaultBRDFInput(
		baseColor,
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		anisotropyDirection,
		clearCoatNormal,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		anisotropy,
		clearCoat,
		clearCoatPerceptualRoughness,
		emission,
		pixelData);
}

void DefaultCharacterMaterialFunction(
	vec3 fragPos,
	uint drawID,
	vec3 uv,
	mat3 TBN,
	vec3 tangent,
	vec3 normal,
	inout PixelData pixelData)
{
	vec3 viewVector = normalize(cameraPosTimeW.xyz - fragPos);

	Material material = GetMaterial(drawID, uv);

	vec3 baseColor = SampleMaterialDiffuse(material, uv).rgb;

	vec3 materialNormal = SampleMaterialNormal(TBN, normal, material, uv).xyz;

	float reflectance = material.reflectance;
	
	float anisotropy = material.anisotropy.color.r;
	
	float metallic = SampleMaterialMetallic(material, uv).r;

	//change from the world version
	float ao = SampleMaterialAO(material, uv).r;

	float perceptualRoughness = clamp(SampleMaterialRoughness(material, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	//change from the world version
	vec3 multibounceAO = vec3(ao);

	vec3 anisotropyDirection = normalize(SampleAnisotropyDirection(TBN, tangent, material, uv));

	float clearCoat = material.clearCoat.color.r;

	vec3 clearCoatNormal = normal;

	float clearCoatPerceptualRoughness = clamp(material.clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;

	DefaultBRDFInput(
		baseColor,
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		anisotropyDirection,
		clearCoatNormal,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		anisotropy,
		clearCoat,
		clearCoatPerceptualRoughness,
		emission,
		pixelData);
}

#endif