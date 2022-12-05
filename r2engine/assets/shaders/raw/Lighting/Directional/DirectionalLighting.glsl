#ifndef GLSL_DIRECTIONAL_LIGHTING
#define GLSL_DIRECTIONAL_LIGHTING

#include "Common/CommonFunctions.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Lighting/PBR/MaterialBRDF.glsl"
#include "Shadows/Directional/DirectionalShadows.glsl"

//Everything turned on
vec3 EvalDirectionalLight(DirLight dirLight, in PixelData pixel)
{
	vec3 L = normalize(-dirLight.direction.xyz);

	float NoL = Saturate(dot(pixel.N, L));

	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity * exposureNearFar.x;

	vec3 result = EvalBRDF(L, NoL, pixel);

	return result * radiance * NoL;
}

void EvalAllDirectionalLights(inout vec3 L0, in PixelData pixel)
{
	for(int i = 0; i < numDirectionLights; ++i)
	{
		DirLight light = dirLights[i];

		vec3 lightingResult = EvalDirectionalLight(light, pixel);

		float shadow = 0.0f;

		if(light.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			shadow = ShadowCalculation(shadowsSurface, pixel.worldFragPos, pixel.N, cameraPosTimeW.xyz, light.direction.xyz, light.lightProperties.lightID, light.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		L0 += lightingResult * (1.0f - shadow);
	}
}

//No clear coat, no anisotropy
vec3 EvalDirectionalLightNoClearCoatNoAnisotropy(DirLight dirLight, in PixelData pixel)
{
	vec3 L = normalize(-dirLight.direction.xyz);

	float NoL = Saturate(dot(pixel.N, L));

	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity * exposureNearFar.x;

	vec3 result = EvalBRDFNoClearCoatNoAnisotropy(L, NoL, pixel);

	return result * radiance * NoL;
}

void EvalAllDirectionalLightsNoClearCoatNoAnisotropy(inout vec3 L0, in PixelData pixel)
{
	for(int i = 0; i < numDirectionLights; ++i)
	{
		DirLight light = dirLights[i];

		vec3 lightingResult = EvalDirectionalLightNoClearCoatNoAnisotropy(light, pixel);

		float shadow = 0.0f;

		if(light.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			shadow = ShadowCalculation(shadowsSurface, pixel.worldFragPos, pixel.N, cameraPosTimeW.xyz, light.direction.xyz, light.lightProperties.lightID, light.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		L0 += lightingResult * (1.0f - shadow);
	}
}

//Clear coat, no anisotropy
vec3 EvalDirectionalLightWithClearCoat(DirLight dirLight, in PixelData pixel)
{
	vec3 L = normalize(-dirLight.direction.xyz);

	float NoL = Saturate(dot(pixel.N, L));

	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity * exposureNearFar.x;

	vec3 result = EvalBRDFWithClearCoat(L, NoL, pixel);

	return result * radiance * NoL;
}

void EvalAllDirectionalLightsWithClearCoat(inout vec3 L0, in PixelData pixel)
{
	for(int i = 0; i < numDirectionLights; ++i)
	{
		DirLight light = dirLights[i];

		vec3 lightingResult = EvalDirectionalLightWithClearCoat(light, pixel);

		float shadow = 0.0f;

		if(light.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			shadow = ShadowCalculation(shadowsSurface, pixel.worldFragPos, pixel.N, cameraPosTimeW.xyz, light.direction.xyz, light.lightProperties.lightID, light.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		L0 += lightingResult * (1.0f - shadow);
	}
}

//With anisotropy
vec3 EvalDirectionalLightWithAnisotropy(DirLight dirLight, in PixelData pixel)
{
	vec3 L = normalize(-dirLight.direction.xyz);

	float NoL = Saturate(dot(pixel.N, L));

	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity * exposureNearFar.x;

	vec3 result = EvalBRDFWithAnisotropy(L, NoL, pixel);

	return result * radiance * NoL;
}

void EvalAllDirectionalLightsWithAnisotropy(inout vec3 L0, in PixelData pixel)
{
	for(int i = 0; i < numDirectionLights; ++i)
	{
		DirLight light = dirLights[i];

		vec3 lightingResult = EvalDirectionalLightWithAnisotropy(light, pixel);

		float shadow = 0.0f;

		if(light.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			shadow = ShadowCalculation(shadowsSurface, pixel.worldFragPos, pixel.N, cameraPosTimeW.xyz, light.direction.xyz, light.lightProperties.lightID, light.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		L0 += lightingResult * (1.0f - shadow);
	}
}

#endif