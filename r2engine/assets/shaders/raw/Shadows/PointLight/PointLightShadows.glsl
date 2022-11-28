#ifndef GLSL_POINTLIGHT_SHADOWS
#define GLSL_POINTLIGHT_SHADOWS

#include "Shadows/ShadowData.glsl"
#include "Shadows/ShadowCommon.glsl"

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);   

float PointLightShadowCalculation(Tex2DAddress shadowTex, vec3 fragPos, vec3 fragToLight, vec3 viewPos, float farPlane, int64_t lightID, bool softShadows)
{
	vec3 V = viewPos - fragPos;
	float viewDistance = length(V);
	if(dot(V, fragToLight) > 0)
	{
		return 0;
	}

	float bias = 0.05;
	float lightDepth = length(fragToLight) - bias;

	float pointLightShadowPage = gPointLightShadowMapPages[int(lightID)];

	if(!softShadows)
	{
		float closestDepth = Sample3DShadowMapDepth(shadowTex, fragToLight, vec3(0), pointLightShadowPage);
		closestDepth *= farPlane;

		return lightDepth > closestDepth ? 1.0 : 0.0;
	}
	
	int samples = 20;
	float shadow = 0.0;
	
	float diskRadius = (1.0 + (viewDistance / farPlane)) / 150.0;

	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = Sample3DShadowMapDepth(shadowTex, fragToLight, sampleOffsetDirections[i]*diskRadius, pointLightShadowPage);
		closestDepth *= farPlane;
		if(lightDepth > closestDepth)
		{
			shadow += 1.0;
		}
	}

	return shadow / (float)samples;
}

#endif