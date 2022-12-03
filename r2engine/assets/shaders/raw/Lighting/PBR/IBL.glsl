#ifndef GLSL_IBL
#define GLSL_IBL

#include "Common/Defines.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Lighting/PBR/PBR.glsl"

vec4 SampleMaterialPrefilteredRoughness(SkyLight theSkylight, vec3 uv, float roughnessValue)
{
	Tex2DAddress addr = theSkylight.prefilteredRoughnessTexture;
	//z is up so we rotate this... not sure if this is right?
	return textureLod(samplerCubeArray(addr.container), vec4(uv.r, uv.b, -uv.g, addr.page), roughnessValue);
}

vec4 SampleLUTDFG(SkyLight theSkylight, vec2 uv)
{
	Tex2DAddress addr = theSkylight.lutDFGTexture;
	return texture(sampler2DArray(addr.container), vec3(uv, addr.page));
}

vec4 SampleSkylightDiffuseIrradiance(SkyLight theSkylight, vec3 uv)
{
	Tex2DAddress addr = theSkylight.diffuseIrradianceTexture;
	//z is up so we rotate this... not sure if this is right?
	return texture(samplerCubeArray(addr.container), vec4(uv.r, uv.b, -uv.g, addr.page));
}

void EvalClearCoatIBL(
	SkyLight theSkylight,
	vec3 clearCoatReflectionVector,
	in PixelData pixel,
	inout vec3 Fd,
	inout vec3 Fr)
{
	float Fc = F_Schlick(0.04, 1.0, pixel.clearCoatNoV) * pixel.clearCoat;
	float attenuation = 1.0 - Fc;
	Fd *= attenuation;
	Fr *= attenuation;

	float specularAO = SpecularAO_Lagarde(pixel.clearCoatNoV, pixel.ao, pixel.clearCoatRoughness);
	Fr += SampleMaterialPrefilteredRoughness(theSkylight, clearCoatReflectionVector, pixel.clearCoatPerceptualRoughness * numPrefilteredRoughnessMips).rgb * (specularAO * Fc);
}

void EvalIBL(SkyLight theSkylight, vec3 multibounceAO, inout PixelData pixel, inout vec3 Fd, inout vec3 Fr)
{
	vec3 diffuseIrradiance = SampleSkylightDiffuseIrradiance(theSkylight, pixel.N).rgb;
	vec3 prefilteredRadiance = SampleMaterialPrefilteredRoughness(theSkylight, pixel.R, pixel.roughness * numPrefilteredRoughnessMips).rgb;
	vec2 dfg = SampleLUTDFG(theSkylight, vec2(pixel.NoV, pixel.roughness)).rg;

	pixel.energyCompensation = CalcEnergyCompensation(pixel.F0, dfg);
	float specularAO =  SpecularAO_Lagarde(pixel.NoV, pixel.ao, pixel.roughness);

	vec3 E = F_SchlickRoughness(pixel.NoV, pixel.F0, pixel.roughness);
	Fr += E * prefilteredRadiance * (specularAO * pixel.energyCompensation);
	
	Fd += pixel.diffuseColor * diffuseIrradiance * (1.0 - E) * multibounceAO;
}


#endif