#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint MAX_NUM_LIGHTS = 50;
const uint NUM_FRUSTUM_SPLITS = 4;
const uint NUM_SIDES_FOR_POINTLIGHT =6;

const float PI = 3.141596;
#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS

layout (local_size_x = NUM_SIDES_FOR_POINTLIGHT, local_size_y = 1, local_size_z = 1) in;

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
	int channel;
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

struct LookAtVectors
{
	vec3 dir;
	vec3 up;
};

const LookAtVectors lookAtVectors[NUM_SIDES_FOR_POINTLIGHT] =
{
	{vec3(1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)},
	{vec3(-1.0, 0.0, 0.0), vec3(0.0, -1.0, 0.0)},
	{vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)},
	{vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, -1.0)},
	{vec3(0.0, 0.0, 1.0), vec3(0.0, -1.0, 0.0)},
	{vec3(0.0, 0.0, -1.0), vec3(0.0, -1.0, 0.0)},
};

void main(void)
{
	int pointLightIndex = int(gl_WorkGroupID.x);
	int side = int(gl_LocalInvocationID.x);

	mat4 lightView = LookAt(pointLights[pointLightIndex].position.xyz, pointLights[pointLightIndex].position.xyz + lookAtVectors[side].dir, lookAtVectors[side].up);

pointLights[pointLightIndex].lightProperties.intensity = 50;
	mat4 lightProj = Projection(PI/2.0, 1, exposureNearFar.y, pointLights[pointLightIndex].lightProperties.intensity);



	pointLights[pointLightIndex].lightSpaceMatrices[side] = lightProj * lightView;
}