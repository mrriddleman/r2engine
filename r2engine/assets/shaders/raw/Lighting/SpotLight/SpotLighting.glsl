#ifndef GLSL_SPOT_LIGHTING
#define GLSL_SPOT_LIGHTING

#include "Common/CommonFunctions.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Lighting/PBR/MaterialBRDF.glsl"
#include "Shadows/SpotLight/SpotLightShadows.glsl"
#include "Clusters/Clusters.glsl"

vec3 EvalSpotLight(SpotLight spotLight, in PixelData pixel)
{
	vec3 posToLight = spotLight.position.xyz - pixel.worldFragPos;

	vec3 L = normalize(posToLight);

	float NoL = Saturate(dot(pixel.N, L));

	float attenuation = GetDistanceAttenuation(posToLight, spotLight.lightProperties.fallOffRadius);

	//calculate the spot angle attenuation

	float theta = dot(L, normalize(-spotLight.direction.xyz));

	float epsilon = max(spotLight.position.w - spotLight.direction.w, 1e-4);

	float spotAngleAttenuation = Saturate((theta - spotLight.direction.w) / epsilon);

	spotAngleAttenuation = spotAngleAttenuation * spotAngleAttenuation;

	vec3 radiance = spotLight.lightProperties.color.rgb * attenuation * spotAngleAttenuation * spotLight.lightProperties.intensity * exposureNearFar.x;

	vec3 lightingResult = EvalBRDF(L, NoL, pixel);

	return lightingResult * radiance * NoL;
}

void EvalAllSpotLights(uint clusterTileIndex, inout vec3 L0, in PixelData pixel)
{
	uint spotLightCount = lightGrid[clusterTileIndex].spotLightCount;

	uint spotLightIndexOffset = lightGrid[clusterTileIndex].spotLightOffset;

	for(int i = 0; i < spotLightCount; ++i)
	{
		uint spotLightIndex = globalLightIndexList[spotLightIndexOffset + i].y;
		
		SpotLight spotLight = spotLights[spotLightIndex];

		vec3 lightingResult = EvalSpotLight(spotLight, pixel);

		float shadow = 0;

		if(spotLight.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			shadow = SpotLightShadowCalculation(shadowsSurface, pixel.worldFragPos, pixel.N, spotLight.direction.xyz, spotLight.lightProperties.lightID, spotLights[i].lightSpaceMatrix, spotLight.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		L0 += lightingResult * (1.0f - shadow);
	}
}

#endif