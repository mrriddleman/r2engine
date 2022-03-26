#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
const uint MAX_NUM_LIGHTS = 50;
const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS

const uint NUM_SIDES_FOR_POINTLIGHT =6;
const uint MAX_POINTLIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH;

const uint MAX_VERTICES = NUM_SIDES_FOR_POINTLIGHT * 3;


#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS

layout (triangles, invocations = MAX_INVOCATIONS_PER_BATCH) in;
layout (triangle_strip, max_vertices = MAX_VERTICES) out;

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

uniform uint pointLightBatch;


out vec4 FragPos; // FragPos from GS (output per emitvertex)
out vec3 LightPos;
out float FarPlane;

void main(void)
{
	int lightIndex = gl_InvocationID  + int(pointLightBatch) * int(MAX_POINTLIGHTS_PER_BATCH);

	if(lightIndex < numPointLights)
	{
		for(int face = 0; face < NUM_SIDES_FOR_POINTLIGHT; ++face)
		{
			gl_Layer = face + int(gPointLightShadowMapPages[int(pointLights[lightIndex].lightProperties.lightID)]) * int(NUM_SIDES_FOR_POINTLIGHT);

			vec4 vertex[3];
			int outOfBounds[6] = {0,0,0,0,0,0};

			for(int i = 0; i < 3; ++i)
			{
				vertex[i] = pointLights[lightIndex].lightSpaceMatrices[face] * gl_in[i].gl_Position;

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
					FragPos = gl_in[i].gl_Position;
					LightPos = pointLights[lightIndex].position.xyz;
					FarPlane = pointLights[lightIndex].lightProperties.intensity; //if we change the compute shader, this needs to change as well
					EmitVertex();
				}

				EndPrimitive();
			}
			
		}
	}
}