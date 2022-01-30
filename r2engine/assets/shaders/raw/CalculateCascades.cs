#version 450 core

#extension GL_NV_gpu_shader5 : enable



//@TODO(Serge): make this into a real thing we can pass in
#define MAX_NUM_LIGHTS 50
#define NUM_FRUSTUM_SPLITS 4
#define NUM_FRUSTUM_CORNERS 8


layout (local_size_x = NUM_FRUSTUM_SPLITS, local_size_y = 1, local_size_z = 1) in;


const float projMult = 1.25;
const float zMult = 10;
const vec3 GLOBAL_UP = vec3(0, 0, 1);

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

void main(void)
{
	uint cascadeIndex = gl_LocalInvocationIndex;

	mat4 projViewInv = MatInverse(cameraFrustumProjections[cascadeIndex] * view);

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

	float diameter = length(frustumCorners[0] - frustumCorners[6]);
	float radius = diameter / 2.0;

	float texelsPerUnit = shadowMapSizes[cascadeIndex] / diameter;

	mat4 scalar = mat4(1.0);
	scalar[0][0] = scalar[0][0] * texelsPerUnit;
	scalar[1][1] = scalar[1][1] * texelsPerUnit;
	scalar[2][1] = scalar[2][2] * texelsPerUnit;

	vec3 baseLookAt = -dirLights[0].direction.xyz;
	vec3 ZERO = vec3(0);

	mat4 lookAtMat = scalar * LookAt(ZERO, baseLookAt, GLOBAL_UP);
	mat4 lookAtMatInv = MatInverse(lookAtMat);

	center = vec3(lookAtMat * vec4(center, 1.0));
	center.x = floor(center.x);
	center.y = floor(center.y);
	center = vec3(lookAtMatInv * vec4(center, 1.0));

	vec3 eye = center - (dirLights[0].direction.xyz * diameter);

	dirLights[0].lightSpaceMatrixData.lightViewMatrices[cascadeIndex] = LookAt(eye, center, GLOBAL_UP);
	
	dirLights[0].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] = Ortho(-radius * projMult, radius * projMult, -radius * projMult, radius * projMult, -radius * zMult, radius * zMult);
	
}