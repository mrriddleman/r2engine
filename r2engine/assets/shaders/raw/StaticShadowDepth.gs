#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_FRUSTUM_SPLITS = 4; 
const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS
const uint MAX_NUM_LIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH / NUM_FRUSTUM_SPLITS;
const uint NUM_SIDES_FOR_POINTLIGHT = 6;

#define MAX_NUM_DIRECTIONAL_LIGHTS 50
#define MAX_NUM_POINT_LIGHTS 4096
#define MAX_NUM_SPOT_LIGHTS MAX_NUM_POINT_LIGHTS
#define MAX_NUM_SHADOW_MAP_PAGES 50
#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_SPOT_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_POINT_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_SHADOW_MAP_PAGES

layout (triangles, invocations = MAX_INVOCATIONS_PER_BATCH) in;
layout (triangle_strip, max_vertices = 3) out;

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

uniform uint directionLightBatch;

void main()
{

	int lightIndex = (gl_InvocationID / int(NUM_FRUSTUM_SPLITS)) + int(directionLightBatch) * int(MAX_NUM_LIGHTS_PER_BATCH);
	int cascadeIndex = gl_InvocationID % int(NUM_FRUSTUM_SPLITS);

	if(lightIndex < numShadowCastingDirectionLights)
	{

		int dirLightIndex = (int)shadowCastingDirectionLights[lightIndex];

		vec3 normal = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz);
		vec3 view = -dirLights[dirLightIndex].direction.xyz;
	//	Partition part = gPartitions[gl_InvocationID];

		if(dot(normal, view) > 0.0f)
		{
			vec4 vertex[3];
			int outOfBound[6] = { 0 , 0 , 0 , 0 , 0 , 0 };
			for (int i =0; i < 3; ++i )
			{
				vertex[i] = dirLights[dirLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] * dirLights[dirLightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex] * gl_in[i].gl_Position;
				

				if ( vertex[i].x > +vertex[i].w ) ++outOfBound[0];
				if ( vertex[i].x < -vertex[i].w ) ++outOfBound[1];
				if ( vertex[i].y > +vertex[i].w ) ++outOfBound[2];
				if ( vertex[i].y < -vertex[i].w ) ++outOfBound[3];
				if ( vertex[i].z > +vertex[i].w ) ++outOfBound[4];
				if ( vertex[i].z < -vertex[i].w ) ++outOfBound[5];
			}

			bool inFrustum = true;
			for (int i = 0; i < 6; ++i )
				if ( outOfBound[i] == 3) inFrustum = false;

			if(inFrustum)
			{
				for(int i = 0; i < 3; ++i)
				{
					gl_Position = vertex[i];
					gl_Layer = cascadeIndex + int(gDirectionLightShadowMapPages[int(dirLights[dirLightIndex].lightProperties.lightID)]);
					EmitVertex();
				}

				EndPrimitive();
			}
		}
	}
}