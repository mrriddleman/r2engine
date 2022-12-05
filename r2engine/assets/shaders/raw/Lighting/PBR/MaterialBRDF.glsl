#ifndef GLSL_MATERIAL_BRDF
#define GLSL_MATERIAL_BRDF

#include "Common/Defines.glsl"
#include "Common/CommonFunctions.glsl"
#include "Lighting/PBR/PBR.glsl"

float ClearCoatLobe(float clearCoat, float clearCoatRoughness, float NoH, float LoH, out float Fcc)
{
	float clearCoatNoH = NoH;

	float Dcc = D_GGX(clearCoatNoH, clearCoatRoughness);
	float Vcc = V_Kelemen(LoH);
	float F = F_Schlick(0.04, 1.0, LoH) * clearCoat;

	Fcc = F;
	return Dcc * Vcc * F;
}

vec3 Base_BRDF_Specular(	
	vec3 N,
	vec3 V,
	vec3 L,
	vec3 H,
	vec3 F0,
	float NoV,
	float NoL,
	float NoH,
	float ggxVTerm,
	vec3 energyCompensation,
	float roughness)
{
	float D = D_GGX(NoH, roughness);
	vec3 F = F_Schlick(max(dot(H, V), 0.0), F0);
	float VSmith = V_SmithGGXCorrelated(NoV, NoL, roughness, ggxVTerm);

	//specular BRDF
	vec3 Fr =  (D * VSmith) * F;

	//Energy compensation
	Fr *= energyCompensation;

	return Fr;
}

vec3 EvalAnisoBRDF(vec3 L, float NoL, vec3 H, float NoH, float LoH, in PixelData pixel)
{
	vec3 T = pixel.anisotropicT ; //TODO(Serge): calculate this by passing in the materials anisotropic direction - probably need to pass in
	vec3 B = pixel.anisotropicB; //TODO(Serge): calc based on anisotropic direction

	float ToL = dot(T, L);
	float BoL = dot(B, L);
	float ToH = dot(T, H);
	float BoH = dot(B, H);

	float Da = D_GGX_Anisotropic(pixel.at, pixel.ab, ToH, BoH, NoH);
	float Va = V_SmithGGXCorrelated_Anisotropic(pixel.at, pixel.ab, pixel.ggxVTerm, ToL, BoL, pixel.NoV, NoL);	
	vec3 F = Fresnel(pixel.F0, LoH);

	vec3 Fr = (Da * Va) * F;
	vec3 kD = vec3(1.0) - F;

	return (kD * pixel.Fd + Fr);
}

vec3 BRDFWithClearCoat(in vec3 L, in float NoL, in vec3 H, in float NoH, in float LoH, in PixelData pixel)
{
	vec3 Fr = Base_BRDF_Specular(pixel.N, pixel.V, L, H, pixel.F0, pixel.NoV, NoL, NoH, pixel.ggxVTerm, pixel.energyCompensation, pixel.roughness);
	
	float clearCoatNoH = Saturate(dot(pixel.clearCoatNormal, H));

	float Fcc;
	float clearCoatLobe = ClearCoatLobe(pixel.clearCoat, pixel.clearCoatRoughness, clearCoatNoH, LoH, Fcc);
	float attenuation = 1.0 - Fcc;

	return ((pixel.Fd + Fr) * attenuation + clearCoatLobe);
}

vec3 BRDFDefault(in vec3 L, in float NoL, in vec3 H, in float NoH, in float LoH, in PixelData pixel)
{
	vec3 Fr = Base_BRDF_Specular(pixel.N, pixel.V, L, H, pixel.F0, pixel.NoV, NoL, NoH, pixel.ggxVTerm, pixel.energyCompensation, pixel.roughness);
	
	return pixel.Fd + Fr;
}

vec3 EvalBRDF(vec3 L, float NoL, in PixelData pixel)
{
	vec3 H = normalize(pixel.V + L);
	float NoH = Saturate(dot(pixel.N, H));
	float LoH = Saturate(dot(L, H));

	if(pixel.anisotropy != 0.0)
	{
		return EvalAnisoBRDF(L, NoL, H, NoH, LoH, pixel);
	}

	return BRDFWithClearCoat(L, NoL, H, NoH, LoH, pixel);
}

vec3 EvalBRDFNoClearCoatNoAnisotropy(vec3 L, float NoL, in PixelData pixel)
{
	vec3 H = normalize(pixel.V + L);
	float NoH = Saturate(dot(pixel.N, H));
	float LoH = Saturate(dot(L, H));

	return BRDFDefault(L, NoL, H, NoH, LoH, pixel);
}


#endif