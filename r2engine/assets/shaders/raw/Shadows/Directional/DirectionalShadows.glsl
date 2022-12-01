#ifndef GLSL_DIR_SHADOW
#define GLSL_DIR_SHADOW

#include "Common/Defines.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Depth/DepthUtils.glsl"
#include "Shadows/ShadowData.glsl"
#include "Shadows/ShadowCommon.glsl"
#include "Shadows/ShadowFilters.glsl"

float SampleShadowCascade(Tex2DAddress shadowTex, vec3 fragPos, vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float NoL, float VoL, bool softShadows)
{
	vec3 viewDir = (cameraPosTimeW.xyz - fragPos);
	vec3 viewDirN = normalize(viewDir);
	float viewDirLen = length(viewDir);

	float xOffset = (viewDir.x / viewDirLen) * (1.0 - VoL);
	float yOffset = (viewDir.y / viewDirLen) * (1.0 - VoL);

	shadowPosition += gBias[cascadeIndex][int(lightID)].xyz;
	shadowPosition *= gScale[cascadeIndex][int(lightID)].xyz;

	if(shadowPosition.z > 1.0)
	{
		return 0.0;
	}

	float lightDepth = shadowPosition.z;
	float bias = max(0.001 * (1.0 - NoL), 0.0005);
	if(cascadeIndex <= NUM_FRUSTUM_SPLITS - 1)
	{
		bias *= 1.0 / (gPartitions.intervalEnd[cascadeIndex] - gPartitions.intervalBegin[cascadeIndex]);
	}

	lightDepth -= bias;

	float directionalLightPage = float(cascadeIndex) + gDirectionLightShadowMapPages[int(lightID)];
	vec2 shadowMapSize = vec2(shadowMapSizes[cascadeIndex]);

	if(softShadows)
	{
		return SoftShadow(shadowTex, shadowPosition, shadowMapSize, directionalLightPage, lightDepth);
	}
	else
	{
		//@TODO(Serge): need some way of switching here

		return OptimizedPCF3(shadowTex, shadowPosition, shadowMapSize, directionalLightPage, lightDepth);
	}
}

vec3 GetShadowPosOffset(float NoL, vec3 normal, uint cascadeIndex)
{
	const float OFFSET_SCALE = 0.0f;

	float texelSize = 2.0 / shadowMapSizes[cascadeIndex];
	float nmlOffsetScale = clamp(1.0 - NoL, 0.0, 1.0);
	return texelSize * OFFSET_SCALE * nmlOffsetScale * normal;
}

float ShadowCalculation(Tex2DAddress shadowTex, vec3 fragPosWorldSpace, vec3 normal, vec3 cameraPos, vec3 lightDir, int64_t lightID, bool softShadows)
{
	//vec3 normal = normalize(fs_in.normal);
	vec3 viewDir = normalize(cameraPos - fragPosWorldSpace);

	//don't put the shadow on the back of a surface
	if(dot(viewDir, normal) < 0.0)
	{
		return 1.0;
	}
	
	int lightIndex = int(lightID);

	float NoL = max(dot(-lightDir, normal), 0.0);
	float VoL = dot(-lightDir, viewDir);

	vec4 projectionPosInCSMSplitSpace = (gShadowMatrix[lightIndex] * vec4(fragPosWorldSpace, 1.0));

	vec3 projectionPos = projectionPosInCSMSplitSpace.xyz / projectionPosInCSMSplitSpace.w;

	uint layer = NUM_FRUSTUM_SPLITS - 1;

	for(int i = int(layer); i >= 0; --i)
	{
		vec3 scale = gScale[i][lightIndex].xyz;
		vec3 bias = gBias[i][lightIndex].xyz;

		vec3 cascadePos = projectionPos + bias;
		cascadePos *= scale;
		cascadePos = abs(cascadePos - 0.5f);
		if(cascadePos.x <= 0.5 && cascadePos.y <= 0.5 && cascadePos.z <= 0.5)
		{
			layer = i;
		}
	}


	vec3 offset = GetShadowPosOffset(NoL, normal, layer);

	vec3 samplePos = fragPosWorldSpace + offset;

	vec4 tempShadowPos = gShadowMatrix[lightIndex] * vec4(samplePos, 1.0);
	vec3 shadowPosition = (tempShadowPos.xyz/tempShadowPos.w);

	float shadowVisibility = SampleShadowCascade(shadowTex, fragPosWorldSpace, shadowPosition, layer, lightID, NoL, VoL, softShadows);

	
	const float BLEND_THRESHOLD = 0.1f;

    float nextSplit =  gPartitions.intervalEnd[layer] - gPartitions.intervalBegin[layer];//gPartitions[layer].intervalEndBias.x;
    float splitSize = layer == 0 ? nextSplit :  gPartitions.intervalEnd[layer-1] - gPartitions.intervalBegin[layer-1];//gPartitions[layer - 1].intervalEndBias.x;
    float fadeFactor = (nextSplit - (LinearizeDepth(gl_FragCoord.z, exposureNearFar.y, exposureNearFar.z)))/splitSize;

    vec3 cascadePos = projectionPos + gBias[layer][lightIndex].xyz;//gPartitions[layer].intervalEndBias.yzw;
    cascadePos *= gScale[layer][lightIndex].xyz;//gPartitions[layer].intervalBeginScale.yzw;
    cascadePos = abs(cascadePos * 2.0 - 1.0);
    float distToEdge = 1.0 - max(max(cascadePos.x, cascadePos.y), cascadePos.z);
    fadeFactor = max(distToEdge, fadeFactor);

    if(fadeFactor <= BLEND_THRESHOLD && layer != NUM_FRUSTUM_SPLITS - 1)
    {
    	vec3 nextCascadeOffset = GetShadowPosOffset(NoL, normal, layer) / abs(gScale[layer+1][lightIndex].z);
    	vec3 nextCascadeShadowPosition = (gShadowMatrix[lightIndex] * vec4(fragPosWorldSpace + nextCascadeOffset, 1.0)).xyz;

    	float nextSplitVisibility = SampleShadowCascade(shadowTex, fragPosWorldSpace, nextCascadeShadowPosition, layer + 1, lightID, NoL, VoL, softShadows);

    	float lerpAmt = smoothstep(0.0, BLEND_THRESHOLD, fadeFactor);

    	if(layer + 1 >= NUM_FRUSTUM_SPLITS - 1)
    	{
			nextSplitVisibility = 0;
			shadowVisibility = 1;
			lerpAmt = 1;
    	}

    	shadowVisibility = mix(nextSplitVisibility, shadowVisibility, lerpAmt);
    }

	return shadowVisibility;
}


#endif