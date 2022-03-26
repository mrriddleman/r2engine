#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
const uint MAX_NUM_LIGHTS = 50;
const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS
const uint NUM_SPOTLIGHT_LAYERS = 1;
const uint MAX_SPOTLIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH / NUM_SPOTLIGHT_LAYERS;
const uint NUM_SIDES_FOR_POINTLIGHT = 6;

#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS

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

	vec4 gScale[NUM_FRUSTUM_SPLITS][MAX_NUM_LIGHTS];
	vec4 gBias[NUM_FRUSTUM_SPLITS][MAX_NUM_LIGHTS];

	mat4 gShadowMatrix[MAX_NUM_LIGHTS];

	float gSpotLightShadowMapPages[NUM_SPOTLIGHT_SHADOW_PAGES];
	float gPointLightShadowMapPages[NUM_POINTLIGHT_SHADOW_PAGES];
	float gDirectionLightShadowMapPages[NUM_DIRECTIONLIGHT_SHADOW_PAGES];
};

uniform uint spotLightBatch;

void main(void)
{
	int lightIndex = gl_InvocationID + int(spotLightBatch) * int(MAX_SPOTLIGHTS_PER_BATCH);

	if(lightIndex < numSpotLights)
	{
		vec3 normal = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz);
		vec3 lightDir = -spotLights[lightIndex].direction.xyz;

		if(spotLights[lightIndex].lightProperties.castsShadowsUseSoftShadows.x > 0 && dot(normal, lightDir) > 0.0)
		{
			vec4 vertex[3];
			int outOfBounds[6] = {0,0,0,0,0,0};

			for(int i = 0; i < 3; ++i)
			{
				vertex[i] = spotLights[lightIndex].lightSpaceMatrix * gl_in[i].gl_Position;

				if ( vertex[i].x > +vertex[i].w ) ++outOfBounds[0];
				if ( vertex[i].x < -vertex[i].w ) ++outOfBounds[1];
				if ( vertex[i].y > +vertex[i].w ) ++outOfBounds[2];
				if ( vertex[i].y < -vertex[i].w ) ++outOfBounds[3];
				if ( vertex[i].z > +vertex[i].w ) ++outOfBounds[4];
				if ( vertex[i].z < -vertex[i].w ) ++outOfBounds[5];
			}

			bool inFrustum = true;
			for(int i = 0; i < 6; ++i)
			{
				if(outOfBounds[i] == 3) inFrustum = false;
			}

			if(inFrustum)
			{
				for(int i = 0; i < 3; ++i)
				{
					gl_Position = vertex[i];
					gl_Layer = int(gSpotLightShadowMapPages[int(spotLights[lightIndex].lightProperties.lightID)]);
					EmitVertex();
				}

				EndPrimitive();
			}
		}
	}
}