#ifndef GLSL_POINT_LIGHTING
#define GLSL_POINT_LIGHTING

#include "Common/CommonFunctions.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Lighting/PBR/MaterialBRDF.glsl"
#include "Shadows/PointLight/PointLightShadows.glsl"
#include "Clusters/Clusters.glsl"

vec3 EvalPointLight(PointLight pointLight, in PixelData pixel)
{
	vec3 posToLight = pointLight.position.xyz - pixel.worldFragPos;

	vec3 L = normalize(posToLight);

	float NoL = Saturate(dot(pixel.N, L));

	float attenuation = GetDistanceAttenuation(posToLight, pointLight.lightProperties.fallOffRadius);

	vec3 radiance = pointLight.lightProperties.color.rgb * attenuation * pointLight.lightProperties.intensity * exposureNearFar.x;

	vec3 lightingResult = EvalBRDF(L, NoL, pixel);

	return lightingResult * radiance * NoL;
}

void EvalAllPointLights(uint clusterTileIndex, inout vec3 L0, in PixelData pixel)
{
	uint pointLightCount = lightGrid[clusterTileIndex].pointLightCount;
	uint pointLightIndexOffset = lightGrid[clusterTileIndex].pointLightOffset;

	for(int i = 0; i < pointLightCount; ++i)
	{
		uint pointLightIndex = globalLightIndexList[pointLightIndexOffset + i].x;
		PointLight pointLight = pointLights[pointLightIndex];

		vec3 lightingResult = EvalPointLight(pointLight, pixel);

		float shadow = 0.0f;

		if(pointLight.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			vec3 fragToPointLight = pixel.worldFragPos - pointLight.position.xyz;
			shadow = PointLightShadowCalculation(pointLightShadowsSurface, pixel.worldFragPos, fragToPointLight, cameraPosTimeW.xyz, pointLight.lightProperties.intensity, pointLight.lightProperties.lightID, pointLight.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		L0 += lightingResult * (1.0f - shadow);
	}
}


#endif