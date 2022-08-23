#version 450 core

#extension GL_NV_gpu_shader5 : enable



//@TODO(Serge): make this into a real thing we can pass in

#define MAX_NUM_DIRECTIONAL_LIGHTS 50
#define MAX_NUM_POINT_LIGHTS 4096
#define MAX_NUM_SPOT_LIGHTS MAX_NUM_POINT_LIGHTS
#define MAX_NUM_SHADOW_MAP_PAGES 50

#define NUM_FRUSTUM_SPLITS 4
#define NUM_FRUSTUM_CORNERS 8

#define NUM_SPOTLIGHT_LAYERS 1
#define NUM_POINTLIGHT_LAYERS 6
#define NUM_DIRECTIONLIGHT_LAYERS NUM_FRUSTUM_SPLITS

#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_SPOT_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_POINT_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_SHADOW_MAP_PAGES

layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;


const vec3 GLOBAL_UP = vec3(0, 0, 1);
const bool STABALIZE_CASCADES = true;
const uint NUM_SIDES_FOR_POINTLIGHT = 6;


struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
    mat4 inverseProjection;
    mat4 inverseView;
    mat4 vpMatrix;
    mat4 prevProjection;
    mat4 prevView;
    mat4 prevVPMatrix;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
	vec4 fovAspectResXResY;
    uint64_t frame;
   	vec2 clusterScaleBias;
	uvec4 tileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
	vec4 jitter; // {currJitterX, currJitterY, prevJitterX, prevJitterY}
};

layout (std140, binding = 3) uniform SDSMParams
{
	vec4 lightSpaceBorder;
	vec4 maxScale;
	vec4 projMultSplitScaleZMultLambda;
	float dilationFactor;
	uint scatterTileDim;
	uint reduceTileDim;
	uint padding;
	vec4 splitScaleMultFadeFactor;
	Tex2DAddress blueNoiseTexture;
};



struct LightProperties
{
	vec4 color;
	uvec4 castsShadowsUseSoftShadows;
	float fallOffRadius;
	float intensity;
//	uint32_t castsShadows;
	int64_t lightID;
};

struct LightSpaceMatrixData
{
	mat4 lightViewMatrices[NUM_FRUSTUM_SPLITS];
	mat4 lightProjMatrices[NUM_FRUSTUM_SPLITS];
};

struct PointLight
{
	LightProperties lightProperties;
	vec4 position;

	mat4 lightSpaceMatrices[NUM_SIDES_FOR_POINTLIGHT];
};

struct DirLight
{
	LightProperties lightProperties;
	vec4 direction;	
	mat4 cameraViewToLightProj;
	LightSpaceMatrixData lightSpaceMatrixData;
};

struct SpotLight
{
	LightProperties lightProperties;
	vec4 position;//w is radius
	vec4 direction;//w is cutoff

	mat4 lightSpaceMatrix;
};

struct SkyLight
{
	LightProperties lightProperties;
	Tex2DAddress diffuseIrradianceTexture;
	Tex2DAddress prefilteredRoughnessTexture;
	Tex2DAddress lutDFGTexture;
//	int numPrefilteredRoughnessMips;
};


layout (std430, binding = 4) buffer Lighting
{
	PointLight pointLights[MAX_NUM_POINT_LIGHTS];
	DirLight dirLights[MAX_NUM_DIRECTIONAL_LIGHTS];
	SpotLight spotLights[MAX_NUM_SPOT_LIGHTS];
	SkyLight skylight;

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int numPrefilteredRoughnessMips;
	int useSDSMShadows;

	int numShadowCastingDirectionLights;
	int numShadowCastingPointLights;
	int numShadowCastingSpotLights;

	int64_t shadowCastingDirectionLights[MAX_NUM_SHADOW_MAP_PAGES];
	int64_t shadowCastingPointLights[MAX_NUM_SHADOW_MAP_PAGES];
	int64_t shadowCastingSpotLights[MAX_NUM_SHADOW_MAP_PAGES];
};


struct Partition
{
	vec4 intervalBegin;
	vec4 intervalEnd;
};

struct UPartition
{
	uvec4 intervalBegin;
	uvec4 intervalEnd;
};

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions;
	UPartition gPartitionsU;

	vec4 gScale[NUM_FRUSTUM_SPLITS][MAX_NUM_SHADOW_MAP_PAGES];
	vec4 gBias[NUM_FRUSTUM_SPLITS][MAX_NUM_SHADOW_MAP_PAGES];

	mat4 gShadowMatrix[MAX_NUM_SHADOW_MAP_PAGES];

	float gSpotLightShadowMapPages[NUM_SPOTLIGHT_SHADOW_PAGES];
	float gPointLightShadowMapPages[NUM_POINTLIGHT_SHADOW_PAGES];
	float gDirectionLightShadowMapPages[NUM_DIRECTIONLIGHT_SHADOW_PAGES];
};

mat4 MatInverse(mat4 mat)
{
	return inverse(mat);
}

mat4 LookAt(vec3 eye, vec3 center, vec3 up)
{

	vec3 f = normalize(center - eye);
	vec3 s = normalize(cross(f, up));
	vec3 u = cross(s, f);

	mat4 result = mat4(1.0);

	result[0][0] = s.x;
	result[1][0] = s.y;
	result[2][0] = s.z;
	result[0][1] = u.x;
	result[1][1] = u.y;
	result[2][1] = u.z;
	result[0][2] = -f.x;
	result[1][2] = -f.y;
	result[2][2] = -f.z;
	result[3][0] = -dot(s, eye);
	result[3][1] = -dot(u, eye);
	result[3][2] = 	dot(f, eye);

	return result;
}

mat4 Ortho(float left, float right, float bottom, float top, float near, float far)
{
	mat4 result = mat4(1.0);

	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[2][2] = -2.0 / (far - near);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	result[3][2] = -(far + near) / (far - near);

	return result;
}

mat4 Ortho_ZO(float left, float right, float bottom, float top, float near, float far)
{
	mat4 result = mat4(1.0);

	result[0][0] = 2.0 / (right - left);
	result[1][1] = 2.0 / (top - bottom);
	result[2][2] = -1.0 / (far - near);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	result[3][2] = -near / (far - near);

	return result;
}

mat4 Projection(float fov, float aspect, float near, float far)
{
	mat4 result = mat4(0.0);

	float tanHalfFovy = tan(fov / 2.0);

	result[0][0] = 1.0 / (aspect * tanHalfFovy);
	result[1][1] = 1.0 / (tanHalfFovy);
	result[2][2] = - (far + near) / (far - near);
	result[2][3] = -1.0;
	result[3][2] = - (2.0 * far * near) / (far - near);


	return result;
}

mat4 Projection_ZO(float fov, float aspect, float near, float far)
{
	mat4 result = mat4(0.0);

	float tanHalfFovy = tan(fov / 2.0);

	result[0][0] = 1.0 / (aspect * tanHalfFovy);
	result[1][1] = 1.0 / (tanHalfFovy);
	result[2][2] = - far / (far - near);
	result[2][3] = -1.0;
	result[3][2] = - (far * near) / (far - near);


	return result;
}



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


	vec3 upDir = GLOBAL_UP;

	

//	vec3 lightCameraPos = center;
//	vec3 lookAt = center - dirLights[0].direction.xyz;
//	mat4 lightView = LookAt(lightCameraPos, lookAt, GLOBAL_UP);

	mat4 shadowCamera = Ortho_ZO(-0.5, 0.5, -0.5, 0.5, 0.1, 100);

	mat4 lightView = LookAt(center + dirLights[directionLightIndex].direction.xyz * -0.5, center, upDir);

	mat4 texScaleBias = mat4(1.0);
	texScaleBias[0][0] = 0.5;
	texScaleBias[1][1] = 0.5;
	texScaleBias[2][2] = 0.5;
	texScaleBias[3][0] = 0.5;
	texScaleBias[3][1] = 0.5;
	texScaleBias[3][2] = 0.5;

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
	float prevSplitDist = cascadeIndex == 0 ? splitScale * min(exposureNearFar.y, (gPartitions.intervalEnd[0] - gPartitions.intervalBegin[0])) : splitScale *  gPartitions.intervalEnd[cascadeIndex-1] - gPartitions.intervalBegin[cascadeIndex-1];
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

	vec3 upDir = GLOBAL_UP;

	vec3 minExtents;
	vec3 maxExtents;
	

	//if(STABALIZE_CASCADES)
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
	// else
	// {
	// 	vec3 lightCameraPos = center;


	// 	mat4 lightView = LookAt(lightCameraPos + dirLights[directionLightIndex].direction.xyz, lightCameraPos, upDir);


	// 	const float MAX_FLOAT = 3.402823466e+38F;
	// 	vec3 mins = vec3(MAX_FLOAT);
	// 	vec3 maxes = vec3(-MAX_FLOAT);

	// 	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	// 	{
	// 		vec3 corner = (lightView * vec4(frustumCorners[i], 1.0)).xyz;
	// 		mins = min(mins, corner);
	// 		maxes = max(maxes, corner);
	// 	}

	// 	minExtents = mins;
	// 	maxExtents = maxes;

	// 	float scale = (shadowMapSizes[cascadeIndex] + 9.0)/shadowMapSizes[cascadeIndex];

	// 	minExtents.x *= scale;
	// 	minExtents.y *= scale;
	// 	maxExtents.x *= scale;
	// 	maxExtents.y *= scale;

	// }
	
	vec3 eye = center + (dirLights[directionLightIndex].direction.xyz) * minExtents.z;

	//float texelsPerUnit = shadowMapSizes[cascadeIndex] / diameter;

	float zMult = projMultSplitScaleZMultLambda.z ;
	float projMult = projMultSplitScaleZMultLambda.x ;
	

	dirLights[directionLightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex] = LookAt(eye, center, upDir);

	dirLights[directionLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] = 
	Ortho_ZO(minExtents.x * projMult, maxExtents.x * projMult, minExtents.y * projMult, maxExtents.y * projMult,
	0, (maxExtents.z - minExtents.z)*zMult);
	

	//Stabalize Cascades

	//if(STABALIZE_CASCADES)
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

	vec3 cascadeScale = 1.0 / vec3(otherCorner - cascadeCorner);


	gScale[cascadeIndex][directionLightLightID] = vec4(cascadeScale.xyz, 1.0);
	gBias[cascadeIndex][directionLightLightID] = vec4(-cascadeCorner.xyz, 0);
}