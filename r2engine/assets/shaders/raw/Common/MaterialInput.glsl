#ifndef GLSL_MATERIAL_INPUT
#define GLSL_MATERIAL_INPUT

#include "Common/Defines.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Lighting/PBR/MaterialSampling.glsl"
#include "Lighting/PBR/PBR.glsl"

void DefaultBRDFInputNoClearCoatNoAnisotropy(
vec3 baseColor,
vec3 diffuseColor,
vec3 worldFragPos,
vec3 materialNormal,
vec3 worldNormal,
vec3 viewVector,
vec3 uv,
vec3 multibounceAO,
float reflectance,
float metallic,
float ao,
float perceptualRoughness,
vec3 emission,
float alpha,
inout PixelData pixel)
{
	pixel.worldFragPos = worldFragPos;
	pixel.baseColor = baseColor;
	pixel.N = materialNormal;
	pixel.V = viewVector;
	pixel.uv = uv;
	pixel.diffuseColor = diffuseColor;
	pixel.alpha = alpha;
	pixel.F0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
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

	pixel.R = GetReflectionVectorNoAnisotropy(pixel.V, pixel.N);

	pixel.emission = emission;
}

void DefaultBRDFInputWithClearCoatAndAnisotropy(
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
float alpha,
inout PixelData pixel)
{
	pixel.worldFragPos = worldFragPos;
	pixel.baseColor = baseColor;
	pixel.N = materialNormal;
	pixel.V = viewVector;
	pixel.uv = uv;
	pixel.diffuseColor = diffuseColor;
	pixel.alpha = alpha;
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

	if(abs(anisotropy) > 1e-5)
	{
		/*
		float ToV = dot(fs_in.tangent, V);
		float BoV = dot(fs_in.bitangent, V);
		*/
		pixel.ggxVTerm = length(vec3(pixel.at * pixel.ToV, pixel.ab * pixel.BoV, pixel.NoV));
	}

	pixel.R = GetReflectionVectorAnisotropy(anisotropy, pixel.anisotropicT, pixel.anisotropicB, perceptualRoughness, pixel.V, pixel.N);

	pixel.clearCoat = clearCoat;
	pixel.clearCoatNormal = worldNormal;
	pixel.clearCoatRoughness = CalculateClearCoatRoughness(clearCoatPerceptualRoughness);
	pixel.clearCoatPerceptualRoughness = clearCoatPerceptualRoughness;
	pixel.clearCoatNoV = max(dot(pixel.clearCoatNormal, pixel.V), 0.0);

	pixel.emission = emission;

}

void DefaultBRDFInputWithClearCoat(
vec3 baseColor,
vec3 diffuseColor,
vec3 worldFragPos,
vec3 materialNormal,
vec3 worldNormal,
vec3 viewVector,
vec3 uv,
vec3 clearCoatNormal,
vec3 multibounceAO,
float reflectance,
float metallic,
float ao,
float perceptualRoughness,
float clearCoat,
float clearCoatPerceptualRoughness,
vec3 emission,
float alpha,
inout PixelData pixel)
{
	pixel.worldFragPos = worldFragPos;
	pixel.baseColor = baseColor;
	pixel.alpha = alpha;
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

	pixel.R = GetReflectionVectorNoAnisotropy(pixel.V, pixel.N);

	pixel.clearCoat = clearCoat;
	pixel.clearCoatNormal = worldNormal;
	pixel.clearCoatRoughness = CalculateClearCoatRoughness(clearCoatPerceptualRoughness);
	pixel.clearCoatPerceptualRoughness = clearCoatPerceptualRoughness;
	pixel.clearCoatNoV = max(dot(pixel.clearCoatNormal, pixel.V), 0.0);

	pixel.emission = emission;
}


void DefaultBRDFInputWithAnisotropy(
vec3 baseColor,
vec3 diffuseColor,
vec3 worldFragPos,
vec3 materialNormal,
vec3 worldNormal,
vec3 viewVector,
vec3 uv,
vec3 anisotropyDirection,
vec3 multibounceAO,
float reflectance,
float metallic,
float ao,
float perceptualRoughness,
float anisotropy,
vec3 emission,
float alpha,
inout PixelData pixel)
{
	pixel.worldFragPos = worldFragPos;
	pixel.baseColor = baseColor;
	pixel.alpha = alpha;
	pixel.N = materialNormal;
	pixel.V = viewVector;
	pixel.uv = uv;
	pixel.diffuseColor = diffuseColor;
	pixel.F0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
	//pixel.F0 = CalculateClearCoatBaseF0(pixel.F0, clearCoat);
	pixel.Fd = diffuseColor * Fd_Lambert();
	pixel.multibounceAO = multibounceAO;
	pixel.reflectance = reflectance;
	pixel.metallic = metallic;
	pixel.ao = ao;
	pixel.roughness = perceptualRoughness * perceptualRoughness;
	pixel.NoV = max(dot(pixel.N, pixel.V), 0.0);
	
	//energy compensation is calculated elsewhere in IBL.glsl

	//float a2 = pixel.roughness * pixel.roughness;
	//pixel.ggxVTerm = sqrt((pixel.NoV - a2 * pixel.NoV) * pixel.NoV + a2);

	pixel.anisotropy = anisotropy;
	pixel.at = max(pixel.roughness * (1.0 + anisotropy), MIN_PERCEPTUAL_ROUGHNESS);
	pixel.ab = max(pixel.roughness * (1.0 - anisotropy), MIN_PERCEPTUAL_ROUGHNESS);
	pixel.anisotropicT = anisotropyDirection;
	pixel.anisotropicB = normalize(cross(worldNormal, anisotropyDirection));
	pixel.ToV = dot(pixel.anisotropicT, pixel.V);
	pixel.BoV = dot(pixel.anisotropicB, pixel.V);

	pixel.ggxVTerm = length(vec3(pixel.at * pixel.ToV, pixel.ab * pixel.BoV, pixel.NoV));
	

	pixel.R = GetReflectionVectorAnisotropy(anisotropy, pixel.anisotropicT, pixel.anisotropicB, perceptualRoughness, pixel.V, pixel.N);

	// pixel.clearCoat = clearCoat;
	// pixel.clearCoatNormal = worldNormal;
	// pixel.clearCoatRoughness = CalculateClearCoatRoughness(clearCoatPerceptualRoughness);
	// pixel.clearCoatPerceptualRoughness = clearCoatPerceptualRoughness;
	// pixel.clearCoatNoV = max(dot(pixel.clearCoatNormal, pixel.V), 0.0);

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

	float NoV = dot(normal, viewVector);
	
	float normalMult = 1.0f;

	if(material.doubleSided > 0 && NoV < 0)
	{
		normalMult = -1.0f;
	}

	normal = normalMult * normal;

	vec4 materialDiffuseColor = SampleMaterialDiffuse(material, uv);

	vec3 baseColor = materialDiffuseColor.rgb * materialDiffuseColor.a;
	float alpha = materialDiffuseColor.a;

	vec3 materialNormal = SampleMaterialNormal(TBN, normal, material, uv).xyz;

	float reflectance = material.reflectance;
	
	float anisotropy = material.anisotropy.color.r;
	
	float metallic = SampleMaterialMetallic(material, uv).r;

	float ao = min(SampleMaterialAO(material, uv).r, SampleAOSurface(gl_FragCoord.xy));

	float perceptualRoughness = clamp(SampleMaterialRoughness(material, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	vec3 multibounceAO = GTAOMultiBounce(ao, diffuseColor);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;



	DefaultBRDFInputNoClearCoatNoAnisotropy(
		
		//use premultiplied alpha
		baseColor,
		
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		emission,

		//alpha
		alpha,

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

	float NoV = dot(normal, viewVector);
	
	float normalMult = 1.0f;

	if(material.doubleSided > 0 && NoV > 0)
	{
		normalMult = -1.0f;
	}

	normal = normalMult * normal;

	vec4 materialDiffuseColor = SampleMaterialDiffuse(material, uv);

	vec3 baseColor = materialDiffuseColor.rgb * materialDiffuseColor.a;

	float alpha = materialDiffuseColor.a;

	vec3 materialNormal = SampleMaterialNormal(TBN, normal, material, uv).xyz;

	float reflectance = material.reflectance;
	
//	float anisotropy = material.anisotropy.color.r;
	
	float metallic = SampleMaterialMetallic(material, uv).r;

	//change from the world version
	float ao = SampleMaterialAO(material, uv).r;

	float perceptualRoughness = clamp(SampleMaterialRoughness(material, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	//change from the world version
	vec3 multibounceAO = vec3(ao);

//	vec3 anisotropyDirection = normalize(SampleAnisotropyDirection(TBN, tangent, material, uv));

//	float clearCoat = material.clearCoat.color.r;

//	vec3 clearCoatNormal = normal;

//	float clearCoatPerceptualRoughness = clamp(material.clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;

	DefaultBRDFInputNoClearCoatNoAnisotropy(
		baseColor,
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		emission,
		//alpha value
		alpha,
		pixelData);
}

void DefaultWorldMaterialFunctionWithClearCoat(
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

	float NoV = dot(normal, viewVector);
	
	float normalMult = 1.0f;

	if(material.doubleSided > 0 && NoV < 0)
	{
		normalMult = -1.0f;
	}

	normal = normalMult * normal;

	vec4 materialDiffuseColor = SampleMaterialDiffuse(material, uv);

	vec3 baseColor = materialDiffuseColor.rgb * materialDiffuseColor.a;

	float alpha = materialDiffuseColor.a;

	vec3 materialNormal = SampleMaterialNormal(TBN, normal, material, uv).xyz;

	float reflectance = material.reflectance;
	
	float anisotropy = material.anisotropy.color.r;
	
	float metallic = SampleMaterialMetallic(material, uv).r;

	float ao = min(SampleMaterialAO(material, uv).r, SampleAOSurface(gl_FragCoord.xy));

	float perceptualRoughness = clamp(SampleMaterialRoughness(material, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	vec3 multibounceAO = GTAOMultiBounce(ao, diffuseColor);

	//vec3 anisotropyDirection = normalize(SampleAnisotropyDirection(TBN, tangent, material, uv));

	//@TODO(Serge): add in the sampling of clear coat materials
	float clearCoat = material.clearCoat.color.r;

	vec3 clearCoatNormal = normal;

	float clearCoatPerceptualRoughness = clamp(material.clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;

	DefaultBRDFInputWithClearCoat(
		baseColor,
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		clearCoatNormal,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		clearCoat,
		clearCoatPerceptualRoughness,
		emission,
		alpha,
		pixelData);
}


void DefaultWorldMaterialFunctionWithAnisotropy(
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

	float NoV = dot(normal, viewVector);
	
	float normalMult = 1.0f;

	if(material.doubleSided > 0 && NoV < 0)
	{
		normalMult = -1.0f;
	}

	normal = normalMult * normal;

	vec4 materialDiffuseColor = SampleMaterialDiffuse(material, uv);

	vec3 baseColor = materialDiffuseColor.rgb * materialDiffuseColor.a;
	float alpha = materialDiffuseColor.a;

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
	//float clearCoat = material.clearCoat.color.r;

	//vec3 clearCoatNormal = normal;

	//float clearCoatPerceptualRoughness = clamp(material.clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;


	DefaultBRDFInputWithAnisotropy(
		baseColor,
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		anisotropyDirection,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		anisotropy,
		emission,
		alpha,
		pixelData);
}

void DefaultCharacterMaterialFunctionWithAnisotropy(
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

	float NoV = dot(normal, viewVector);
	
	float normalMult = 1.0f;

	if(material.doubleSided > 0 && NoV < 0)
	{
		normalMult = -1.0f;
	}

	normal = normalMult * normal;

	vec4 materialDiffuseColor = SampleMaterialDiffuse(material, uv);

	vec3 baseColor = materialDiffuseColor.rgb * materialDiffuseColor.a;
	float alpha = materialDiffuseColor.a;

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

//	float clearCoat = material.clearCoat.color.r;

//	vec3 clearCoatNormal = normal;

//	float clearCoatPerceptualRoughness = clamp(material.clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	vec3 emission = SampleMaterialEmission(material, uv).rgb;

	DefaultBRDFInputWithAnisotropy(
		baseColor,
		diffuseColor,
		fragPos,
		materialNormal,
		normal,
		viewVector,
		uv,
		anisotropyDirection,
		multibounceAO,
		reflectance,
		metallic,
		ao,
		perceptualRoughness,
		anisotropy,
		emission,
		alpha,
		pixelData);
}

#endif