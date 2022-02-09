#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_FRUSTUM_SPLITS = 4; 
const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 50;
const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS
const uint MAX_NUM_LIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH / NUM_FRUSTUM_SPLITS;


#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS

layout (triangles, invocations = MAX_INVOCATIONS_PER_BATCH) in;
layout (triangle_strip, max_vertices = 3) out;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
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

layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
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

uniform uint directionLightBatch;

void main()
{

	int lightIndex = (gl_InvocationID / int(NUM_FRUSTUM_SPLITS)) + int(directionLightBatch);
	int cascadeIndex = gl_InvocationID % int(NUM_FRUSTUM_SPLITS);

	if(lightIndex < numDirectionLights)
	{
		vec3 normal = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz);
		vec3 view = -dirLights[lightIndex].direction.xyz;
	//	Partition part = gPartitions[gl_InvocationID];

		if(dirLights[lightIndex].lightProperties.castsShadowsUseSoftShadows.x > 0 && dot(normal, view) > 0.0f)
		{
			vec4 vertex[3];
			int outOfBound[6] = { 0 , 0 , 0 , 0 , 0 , 0 };
			for (int i =0; i < 3; ++i )
			{
				vertex[i] = dirLights[lightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] * dirLights[lightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex] * gl_in[i].gl_Position;
				

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
					gl_Layer = cascadeIndex + int(gDirectionLightShadowMapPages[int(dirLights[lightIndex].lightProperties.lightID)]);
					EmitVertex();
				}

				EndPrimitive();
			}
		}
	}
}