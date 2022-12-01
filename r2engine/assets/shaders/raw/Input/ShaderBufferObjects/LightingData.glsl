#ifndef GLSL_LIGHTING_DATA
#define GLSL_LIGHTING_DATA

#include "Common/Defines.glsl"
#include "Common/Texture.glsl"

struct LightProperties
{
	vec4 color;
	uvec4 castsShadowsUseSoftShadows;
	float fallOffRadius;
	float intensity;
	//uint32_t castsShadows;
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
//	
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

#endif