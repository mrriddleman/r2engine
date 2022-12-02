#ifndef GLSL_PBR
#define GLSL_PBR

#include "Common/Defines.glsl"
#include "Common/CommonFunctions.glsl"

#define MIN_PERCEPTUAL_ROUGHNESS 0.045

float D_GGX(float NoH, float roughness)
{
	float oneMinusNoHSquared = 1.0 - NoH * NoH;

	float a = NoH * roughness;
	float k = roughness / (oneMinusNoHSquared + a * a);
	float d = k * k * (1.0 / PI);
	return Saturate(d);
}

float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH)
{
	// Burley 2012, "Physically-Based Shading at Disney"

	highp float a2 = at * ab;
	highp vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
	highp float d2 = dot(d, d);
	highp float b2 = a2 / d2;
	return a2 * b2 * b2 * (1.0 / PI);
}

vec3 F_Schlick(float LoH, vec3 F0)
{
	return F0 + (vec3(1.0) - F0) * pow(1.0 - LoH, 5.0);
}

float F_Schlick(float f0, float f90, float VoH)
{
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

vec3 F_Schlick(const vec3 F0, float F90, float VoH)
{
	return F0 + (F90 - F0) * pow(1.0 - VoH, 5.0);
}

vec3 Fresnel(const vec3 f0, float LoH) 
{
    float f90 = Saturate(dot(f0, vec3(50.0 * 0.33)));
    return F_Schlick(f0, f90, LoH);
}

vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness, float ggxVTerm)
{
	float a2 = roughness * roughness;
	float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
	float GGXV = NoL * ggxVTerm;
	return Saturate(0.5 / (GGXV + GGXL));
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float lambdaVTerm, float ToL, float BoL, float NoV, float NoL)
{
	float lambdaV = NoL * lambdaVTerm;
	float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
	float v = 0.5 / (lambdaV + lambdaL);
	return Saturate(v);
}

float V_Kelemen(float LoH)
{
	return Saturate(0.25 / (LoH * LoH));
}

float Fd_Lambert()
{
	return 1.0 / PI;
}

float Fd_Burley(float roughness, float NoV, float NoL, float LoH) 
{
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(1.0, f90, NoL);
    float viewScatter  = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

vec3 CalculateClearCoatBaseF0(vec3 F0, float clearCoat)
{
	vec3 newF0 = Saturate(F0 * (F0 * (0.941892 - 0.263008 * F0) + 0.346479) - 0.0285998);
	return mix(F0, newF0, clearCoat);
}

float CalculateClearCoatRoughness(float clearCoatPerceptualRoughness)
{
	return clearCoatPerceptualRoughness * clearCoatPerceptualRoughness;
}

vec3 CalcEnergyCompensation(vec3 F0, vec2 dfg)
{
	if(dfg.y == 0)
	{
		return vec3(1);
	}
	return 1.0 + F0 * (1.0 / dfg.y - 1.0);
}

vec3 SpecularDFG(vec3 F0, vec2 dfg)
{
	return mix(dfg.xxx, dfg.yyy, F0);
}

float SpecularAO_Lagarde(float NoV, float visibility, float roughness) {
    // Lagarde and de Rousiers 2014, "Moving Frostbite to PBR"
    return Saturate(pow(NoV + visibility, exp2(-16.0 * roughness - 1.0)) - 1.0 + visibility);
}

vec3 GetReflectionVector(float anisotropy, const vec3 anisotropyT, const vec3 anisotropyB, float perceptualRoughness, const vec3 V, const vec3 N)
{
	if(anisotropy != 0.0)
	{
		vec3 anisotropyDirection = anisotropy >= 0.0 ? anisotropyB : anisotropyT;
		vec3 anisotropicTangent = cross(anisotropyDirection, V);
		vec3 anisotripicNormal = cross(anisotropicTangent, anisotropyDirection);
		float bendFactor = abs(anisotropy) * Saturate(5.0 * perceptualRoughness);
		vec3 bentNormal = normalize(mix(N, anisotripicNormal, bendFactor));

		return reflect(-V, bentNormal);
	}

	return reflect(-V, N);
}

vec3 GTAOMultiBounce(float visibility, vec3 albedo)
{
	vec3 a = 2.0404 * albedo - 0.3324;
	vec3 b = -4.7951 * albedo + 0.6417;
	vec3 c = 2.7551 * albedo + 0.6903;
	float x = visibility;

	return max(vec3(x), ((x * a + b) * x + c) * x);
}

#endif