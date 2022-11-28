#ifndef GLSL_ADAPTIVE_DEPTH_BIAS
#define GLSL_ADAPTIVE_DEPTH_BIAS

vec4 CalculatePotentialOccluder(vec3 projCoords, vec2 texelOffset, vec4 lsFragPos, vec3 n, float viewBound, mat4 lightProj, float lightNear)
{
	float delta = 1.0 / shadowMapSizes.x;
	
	vec2 index = floor(vec2((projCoords.xy + delta * texelOffset) * shadowMapSizes.x));

	vec2 nlsGridCenter = delta * (index + vec2(0.5));
	
	vec2 lsGridCenter = viewBound * (2.0 * nlsGridCenter - vec2(1.0)); 

	vec3 lsGridLineDir = normalize(vec3(lsGridCenter, -lightNear));

	float lsTHit = dot(n, lsFragPos.xyz) / dot(n, lsGridLineDir);

	vec3 lsHitP = lsTHit * lsGridLineDir;

	vec4 lsPotentialOccluder = lightProj * vec4(lsHitP, 1.0);

	lsPotentialOccluder = lsPotentialOccluder / lsPotentialOccluder.w;

	lsPotentialOccluder = 0.5 * lsPotentialOccluder + vec4(0.5, 0.5, 0.5, 0.0);

	return lsPotentialOccluder;
}

float CalculateAdaptiveBiasEpsilon(float A, float B, float scaleFactor, float depth, float sceneScale)
{
	return 0.5 * pow(1.0 - A - 2.0 * depth, 2) * scaleFactor * sceneScale / B;
}

float IsLit(float smDepth, float actualDepth, float adaptiveBias, vec3 projCoords, vec3 n, vec3 lsFragPos)
{
	float isLit = smDepth < actualDepth + adaptiveBias ? 1.0 : 0.0;

	isLit = dot(n, lsFragPos) > 0.0001 ? isLit : 0.0;

	isLit = clamp(projCoords.xyz, 0.0, 1.0) != projCoords.xyz ? 0.0 : isLit;

	return isLit;
}

float SpotLightPCFAdaptiveDepthBias(vec3 projCoords, vec3 fragPosWorldSpace, vec3 lightDir, int64_t lightID, int index)
{
	SpotLight spotLight = spotLights[index];

	mat4 lightView = LookAt(spotLight.position.xyz, spotLight.position.xyz + spotLight.direction.xyz, GLOBAL_UP);
	
	//@NOTE(Serge): the 1 here is the aspect ratio - since we're using a size of shadowMapSizes.x x shadowMapSizes.x it will be 1. If this changes, we need to change this as well
	mat4 lightProj = Projection(acos(spotLight.direction.w), 1, exposureNearFar.y, spotLight.lightProperties.intensity);

	float A = lightProj[2][2];
    float B = lightProj[3][2]; 

	vec3 n = normalize((lightView * vec4(fs_in.worldNormal.xyz, 1.0)).xyz);

	vec3 lsLightDir = (lightView * vec4(lightDir, 0.0)).xyz;

	float nDotLsLightDir = dot(n, lsLightDir);
	float nDotLsLightDir2 = nDotLsLightDir * nDotLsLightDir;

	float scaleFactor = min(1.0 / nDotLsLightDir2, 100.0);

	float lightNear = 1;

	float viewBound = lightNear;//* tan(acos(spotLight.direction.w) * (PI / 360.0) ) ;

	vec4 lsFragPos = lightView * vec4(fragPosWorldSpace, 1);

	vec4 Foriginal = CalculatePotentialOccluder(projCoords, vec2(0), lsFragPos, n, viewBound, lightProj, lightNear);

	vec4 Fxp1 = CalculatePotentialOccluder(projCoords, vec2(2, 0), lsFragPos, n, viewBound, lightProj, lightNear);

	vec4 Fyp1 = CalculatePotentialOccluder(projCoords, vec2(0, 2), lsFragPos, n, viewBound, lightProj, lightNear);

	float biasX = Fxp1.z - Foriginal.z;
	float biasY = Fyp1.z - Foriginal.z;
	
	vec2 texelSize = 1.0 / vec2(shadowMapSizes.x);
	float shadowTests = 0.0;
	float shadow = 0.0;
	float sceneScale = 0.01;
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float Fo = Foriginal.z + biasX * x + biasY * y;

			float actualDepth = min(Fo, projCoords.z);

			float smDepth = SampleSpotlightShadowMapDepth(projCoords.xy, x, y, texelSize, lightID);

			float adaptiveBias = CalculateAdaptiveBiasEpsilon(A, B, scaleFactor, smDepth, sceneScale);

			float isLit = IsLit(smDepth, actualDepth, adaptiveBias, projCoords, n, lsFragPos.xyz);

			shadow += isLit;

			shadowTests += 1.0;
		}
	}

	return shadow / shadowTests;
}

#endif