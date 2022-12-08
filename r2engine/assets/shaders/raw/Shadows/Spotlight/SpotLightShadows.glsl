#ifndef GLSL_SPOTLIGHT_SHADOWS
#define GLSL_SPOTLIGHT_SHADOWS

#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"
#include "Shadows/ShadowCommon.glsl"
#include "Shadows/ShadowFilters.glsl"

float SpotLightShadowCalculation(Tex2DAddress shadowTex, vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir, int64_t lightID, mat4 lightSpaceMatrix, bool softShadows)
{
	vec3 viewDir = normalize(cameraPosTimeW.xyz - fragPosWorldSpace);

	//don't put the shadow on the back of a surface
	if(dot(viewDir, normal) < 0.0)
	{
		return 1.0;
	}

	float NoL = max(dot(-lightDir, normal), 0.0);

	vec4 posLightSpace = lightSpaceMatrix * vec4(fragPosWorldSpace, 1);

	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;

	projCoords = projCoords * 0.5 + 0.5;

	float bias = max(0.001 * (1.0 - NoL), 0.001);

	float lightDepth = projCoords.z - bias;

	float spotLightShadowPage = gSpotLightShadowMapPages[int(lightID)];

	if(softShadows)
	{
		return SoftShadow(shadowTex, projCoords, vec2(shadowMapSizes[0]), spotLightShadowPage, lightDepth);
	}


	//return SpotLightPCFAdaptiveDepthBias(projCoords, fragPosWorldSpace, lightDir, lightID, index);
	return OptimizedPCF3(shadowTex, projCoords, vec2(shadowMapSizes[0]), spotLightShadowPage, lightDepth);
}


#endif