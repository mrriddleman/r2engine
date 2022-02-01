#version 450 core

#extension GL_NV_gpu_shader5 : enable



//@TODO(Serge): make this into a real thing we can pass in
#define MAX_NUM_LIGHTS 50 
#define NUM_FRUSTUM_SPLITS 4
#define NUM_FRUSTUM_CORNERS 8


layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;


const float projMult = 1;
const float zMult = 3;
const vec3 GLOBAL_UP = vec3(0, 0, 1);

struct Partition
{
	vec4 intervalBeginScale;
	vec4 intervalEndBias;
};

struct UPartition
{
	uvec4 intervalBeginMinCoord;
	uvec4 intervalEndMaxCoord;
};


layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspect;
};

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct LightProperties
{
	vec4 color;
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
	PointLight pointLights[MAX_NUM_LIGHTS];
	DirLight dirLights[MAX_NUM_LIGHTS];
	SpotLight spotLights[MAX_NUM_LIGHTS];
	SkyLight skylight;

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int numPrefilteredRoughnessMips;
	int useSDSMShadows;
};

layout (std430, binding = 6) buffer ShadowData
{
	Partition gPartitions[NUM_FRUSTUM_SPLITS];
	UPartition gPartitionsU[NUM_FRUSTUM_SPLITS];
	mat4 gShadowMatrix;
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

vec3 GetCameraRight()
{
	vec3 cameraRight;

	cameraRight.x = view[0][0];
	cameraRight.y = view[0][1];
	cameraRight.z = view[0][2];

	return cameraRight;
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


mat4 MakeGlobalShadowMatrix()
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


//	vec3 lightCameraPos = center;
//	vec3 lookAt = center - dirLights[0].direction.xyz;
//	mat4 lightView = LookAt(lightCameraPos, lookAt, GLOBAL_UP);

	mat4 shadowCamera = Ortho(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5);

	mat4 lightView = LookAt(center + dirLights[0].direction.xyz * -0.5, center, GLOBAL_UP);

	mat4 texScaleBias = mat4(0.5);
	//texScaleBias[2][2] = 0.5;
	texScaleBias[3][0] = 0.5;
	texScaleBias[3][1] = 0.5;
	texScaleBias[3][2] = 0.5;

	return texScaleBias * shadowCamera * lightView;
}

void main(void)
{
	uint cascadeIndex = gl_LocalInvocationIndex;


	if(cascadeIndex == 0)
	{
		GlobalShadowMatrix = MakeGlobalShadowMatrix();
		gShadowMatrix = GlobalShadowMatrix;
	}

	barrier();
	memoryBarrierShared();


	mat4 proj = Projection(fovAspect.x, fovAspect.y, gPartitions[cascadeIndex].intervalBeginScale.x, gPartitions[cascadeIndex].intervalEndBias.x );

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


	vec3 center = vec3(0.0);
	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	{
		center += frustumCorners[i];
	}

	center *= (1.0 / NUM_FRUSTUM_CORNERS);

	float sphereRadius = 0.0f;

	for(int i = 0; i < NUM_FRUSTUM_CORNERS; ++i)
	{
		float dist = length(frustumCorners[i] - center);
		sphereRadius = max(sphereRadius, dist);
	}

	sphereRadius = ceil(sphereRadius * 16.0f) / 16.0f;

	float diameter = sphereRadius * 2.0;
	float radius = sphereRadius;


	float texelsPerUnit = shadowMapSizes[cascadeIndex] / diameter;


	vec3 eye = center - (dirLights[0].direction.xyz * diameter);

	dirLights[0].lightSpaceMatrixData.lightViewMatrices[cascadeIndex] = LookAt(eye, center, GLOBAL_UP);

	dirLights[0].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] = Ortho(-radius * projMult, radius *projMult, -radius *projMult, radius * projMult, -zMult, radius * zMult);
	

	//Stabalize Cascades

	mat4 shadowMatrix = dirLights[0].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] * dirLights[0].lightSpaceMatrixData.lightViewMatrices[cascadeIndex];

	vec3 shadowOrigin = vec3(0);
	shadowOrigin = (shadowMatrix * vec4(shadowOrigin, 1.0)).xyz;
	shadowOrigin *= shadowMapSizes[cascadeIndex] / 2.0;

	vec3 roundedOrigin = round(shadowOrigin);
	vec3 roundOffset = roundedOrigin - shadowOrigin;
	roundOffset = roundOffset * (2.0 / shadowMapSizes[cascadeIndex]);
	roundOffset.z = 0.0;

	dirLights[0].lightSpaceMatrixData.lightProjMatrices[cascadeIndex][3][0] += roundOffset.x;
	dirLights[0].lightSpaceMatrixData.lightProjMatrices[cascadeIndex][3][1] += roundOffset.y;

	//Do all the offset and scale calculations


	mat4 shadowProjInv = MatInverse(dirLights[0].lightSpaceMatrixData.lightProjMatrices[cascadeIndex]);
	mat4 shadowViewInv = MatInverse(dirLights[0].lightSpaceMatrixData.lightViewMatrices[cascadeIndex]);
	

	mat4 texScaleBias = mat4(1.0);
	texScaleBias[0][0] = 0.5;
	texScaleBias[1][1] = 0.5;
	texScaleBias[2][2] = 0.5;
	texScaleBias[3][0] = 0.5;
	texScaleBias[3][1] = 0.5;
	texScaleBias[3][2] = 0.5;

	mat4 texScaleBiasInv = MatInverse(texScaleBias);

	mat4 invCascadeMat = shadowViewInv * shadowProjInv * texScaleBiasInv;
	vec3 cascadeCorner = (invCascadeMat * vec4(0, 0, 0, 1)).xyz;
	cascadeCorner = (GlobalShadowMatrix * vec4(cascadeCorner, 1)).xyz;

	vec3 otherCorner = (invCascadeMat * vec4(1, 1, 1, 1)).xyz;
	otherCorner = (GlobalShadowMatrix * vec4(otherCorner, 1)).xyz;

	vec3 cascadeScale = 1.0 / (otherCorner - cascadeCorner);

	gPartitions[cascadeIndex].intervalBeginScale.y = cascadeScale.x;
	gPartitions[cascadeIndex].intervalBeginScale.z = cascadeScale.y;
	gPartitions[cascadeIndex].intervalBeginScale.w = cascadeScale.z;

	gPartitions[cascadeIndex].intervalEndBias.y = -cascadeCorner.x;
	gPartitions[cascadeIndex].intervalEndBias.z = -cascadeCorner.y;
	gPartitions[cascadeIndex].intervalEndBias.w = -cascadeCorner.z;

}