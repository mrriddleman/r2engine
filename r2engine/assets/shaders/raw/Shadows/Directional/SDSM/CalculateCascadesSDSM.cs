#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Common/Texture.glsl"
#include "Common/CommonFunctions.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/SDSMParams.glsl"

#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;

const vec3 SHADOW_GLOBAL_UP = vec3(0, 1, 0);
const bool STABALIZE_CASCADES = true;

vec3 GetCameraRight()
{
	vec3 cameraRight;

	mat3 newView = mat3(view);

	cameraRight.x = newView[0][0];
	cameraRight.y = newView[1][0];
	cameraRight.z = newView[2][0];

	return cameraRight;
}

vec3 GetCameraUp()
{
	vec3 cameraUp;

	cameraUp.x = view[1][0];
	cameraUp.y = view[1][1];
	cameraUp.z = view[1][2];

	return cameraUp;
}

vec4 PlaneFromPoints(in vec3 point1, in vec3 point2, in vec3 point3)
{
	vec3 v21 = point1 - point2;
	vec3 v31 = point1 - point3;

	vec3 n = normalize(cross(v31, v21));
	float d = -dot(n, point1);

	return vec4(n, d);
}

shared mat4 GlobalShadowMatrix;


mat4 MakeGlobalShadowMatrix(int directionLightIndex)
{
	mat4 projViewInv = MatInverse(projection * view);

	vec3 frustumCorners[NUM_FRUSTUM_CORNERS] =
	{
		{-1.0, 1.0, -1.0},
		{1.0, 1.0, -1.0},
		{1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0},
		{-1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0},
		{1.0, -1.0, 1.0},
		{-1.0, -1.0, 1.0}
	};

	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	{
		vec4 pt = projViewInv * vec4(frustumCorners[i], 1.0);
		frustumCorners[i] = pt.xyz / pt.w;
	}

	vec3 center = vec3(0.0);
	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	{
		center += frustumCorners[i];
	}

	center *= (1.0 / NUM_FRUSTUM_CORNERS);


	vec3 upDir = GetCameraRight();
	if(STABALIZE_CASCADES)
	{
		upDir = SHADOW_GLOBAL_UP;
	}

//	vec3 lightCameraPos = center;
//	vec3 lookAt = center - dirLights[0].direction.xyz;
//	mat4 lightView = LookAt(lightCameraPos, lookAt, GLOBAL_UP);

	mat4 shadowCamera = Ortho_ZO(-0.5, 0.5, -0.5, 0.5, 0.1, 1000);

	mat4 lightView = LookAt(center + dirLights[directionLightIndex].direction.xyz * -0.5, center, upDir);

	mat4 texScaleBias = mat4(1.0);
	texScaleBias[0][0] = 1;
	texScaleBias[1][1] = 1;
	texScaleBias[2][2] = 1;
	texScaleBias[3][0] = 0;
	texScaleBias[3][1] = 0;
	texScaleBias[3][2] = 0;

	return texScaleBias * shadowCamera * lightView;
}

int GetCurrentDirectionLightLightID(int dirLightIndex)
{
	return int(dirLights[dirLightIndex].lightProperties.lightID);
}

void main(void)
{
	uint cascadeIndex = gl_LocalInvocationID.x;
	int directionLightIndex = (int)shadowCastingDirectionLights[int(gl_WorkGroupID.x)];
	int directionLightLightID = GetCurrentDirectionLightLightID(directionLightIndex);


	if(cascadeIndex == 0)
	{
		GlobalShadowMatrix = MakeGlobalShadowMatrix(directionLightIndex);
		gShadowMatrix[directionLightLightID] = GlobalShadowMatrix;
	}

	barrier();
	memoryBarrierShared();

	mat4 proj = Projection(fovAspectResXResY.x, fovAspectResXResY.y, gPartitions.intervalBegin[cascadeIndex], gPartitions.intervalEnd[cascadeIndex] );

	mat4 projViewInv = MatInverse(proj * view);

	vec3 frustumCorners[NUM_FRUSTUM_CORNERS] =
	{
		{-1.0, 1.0, -1.0},
		{1.0, 1.0, -1.0},
		{1.0, -1.0, -1.0},
		{-1.0, -1.0, -1.0},
		{-1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0},
		{1.0, -1.0, 1.0},
		{-1.0, -1.0, 1.0}
	};

	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	{
		vec4 pt = projViewInv * vec4(frustumCorners[i], 1.0);
		frustumCorners[i] = pt.xyz / pt.w;
	}

	float splitScale = projMultSplitScaleZMultLambda.y * splitScaleMultFadeFactor.x;
	float prevSplitDist = cascadeIndex == 0 ? splitScale * min(1, (gPartitions.intervalEnd[0] - gPartitions.intervalBegin[0])) : splitScale *  gPartitions.intervalEnd[cascadeIndex-1] - gPartitions.intervalBegin[cascadeIndex-1];
	float splitDist =  (gPartitions.intervalEnd[cascadeIndex] - gPartitions.intervalBegin[cascadeIndex]) * ( splitScaleMultFadeFactor.x / splitScale);	

	for(int i = 0; i < 4; ++i)
    {
         vec3 cornerRay = frustumCorners[i + 4] - frustumCorners[i];
         vec3 nearCornerRay = (cornerRay * prevSplitDist) * splitScale;
         vec3 farCornerRay = (cornerRay * splitDist) * splitScale;
         frustumCorners[i + 4] = frustumCorners[i] + farCornerRay;
         frustumCorners[i] = frustumCorners[i] + nearCornerRay;
    }

	vec3 center = vec3(0.0);
	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	{
		center += frustumCorners[i];
	}

	center *= (1.0 / NUM_FRUSTUM_CORNERS);

	vec3 upDir = GetCameraRight();

	if(STABALIZE_CASCADES)
		upDir = SHADOW_GLOBAL_UP;

	vec3 minExtents;
	vec3 maxExtents;
	

	if(STABALIZE_CASCADES)
	{
		float sphereRadius = 0.0;

		for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
		{
			float dist = length(frustumCorners[i] - center) ;
			sphereRadius = max(sphereRadius, dist);
		}

		sphereRadius = ceil(sphereRadius * 16.0) / 16.0;

		//float diameter = sphereRadius * 2.0;
		
		

		maxExtents = vec3(sphereRadius);
		minExtents = -maxExtents;

	}
	else
	{
		vec3 lightCameraPos = center;


		mat4 lightView = LookAt(lightCameraPos + dirLights[directionLightIndex].direction.xyz, lightCameraPos, upDir);


		const float MAX_FLOAT = 3.402823466e+38F;
		vec3 mins = vec3(MAX_FLOAT);
		vec3 maxes = vec3(-MAX_FLOAT);

		for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
		{
			vec3 corner = (lightView * vec4(frustumCorners[i], 1.0)).xyz;
			mins = min(mins, corner);
			maxes = max(maxes, corner);
		}

		minExtents = mins;
		maxExtents = maxes;

		float scale = (shadowMapSizes[cascadeIndex] + 9.0)/shadowMapSizes[cascadeIndex];

		minExtents.x *= scale;
		minExtents.y *= scale;
		maxExtents.x *= scale;
		maxExtents.y *= scale;

	}
	
	vec3 eye = center + (dirLights[directionLightIndex].direction.xyz) * minExtents.z;

	//float texelsPerUnit = shadowMapSizes[cascadeIndex] / diameter;

	float zMult = projMultSplitScaleZMultLambda.z ;
	float projMult = projMultSplitScaleZMultLambda.x ;
	

	dirLights[directionLightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex] = LookAt(eye, center, upDir);

	dirLights[directionLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] = 
	Ortho_ZO(minExtents.x * projMult, maxExtents.x * projMult, minExtents.y * projMult, maxExtents.y * projMult,
	0, (maxExtents.z - minExtents.z)*zMult);
	

	//Stabalize Cascades
	
	if(STABALIZE_CASCADES)
	{
		mat4 shadowMatrix = dirLights[directionLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] * dirLights[directionLightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex];

		vec3 shadowOrigin = vec3(0);
		shadowOrigin = (shadowMatrix * vec4(shadowOrigin, 1.0)).xyz;
		shadowOrigin *= shadowMapSizes[cascadeIndex] / 2.0;

		vec3 roundedOrigin = round(shadowOrigin);
		vec3 roundOffset = roundedOrigin - shadowOrigin;
		roundOffset = roundOffset * (2.0 / shadowMapSizes[cascadeIndex]);
		roundOffset.z = 0.0;

		dirLights[directionLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex][3][0] += roundOffset.x;
		dirLights[directionLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex][3][1] += roundOffset.y;
	}
	

	//Do all the offset and scale calculations


	mat4 shadowProjInv = MatInverse(dirLights[directionLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex]);
	mat4 shadowViewInv = MatInverse(dirLights[directionLightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex]);
	

	mat4 texScaleBias = mat4(1.0);
	texScaleBias[0][0] = 0.5;
	texScaleBias[1][1] = 0.5;
	texScaleBias[2][2] = 0.5;
	texScaleBias[3][0] = 0.5;
	texScaleBias[3][1] = 0.5;
	texScaleBias[3][2] = 0.5;

	mat4 texScaleBiasInv = MatInverse(texScaleBias);

	mat4 invCascadeMat = shadowViewInv * shadowProjInv * texScaleBiasInv;
	vec4 cascadeCorner = GlobalShadowMatrix * (invCascadeMat * vec4(0, 0, 0, 1));
	
	cascadeCorner /= cascadeCorner.w;

	vec4 otherCorner = GlobalShadowMatrix * (invCascadeMat * vec4(1, 1, 1, 1));
	otherCorner /= otherCorner.w;

	vec3 cascadeScale = 1.0 / (otherCorner.xyz - cascadeCorner.xyz);


	gScale[cascadeIndex][directionLightLightID] = vec4(cascadeScale.xyz, 1.0);
	gBias[cascadeIndex][directionLightLightID] = vec4(-cascadeCorner.xyz, 0);
}