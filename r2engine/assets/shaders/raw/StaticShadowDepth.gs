#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4 //TODO(Serge): pass in
const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 50;

layout (triangles, invocations = NUM_FRUSTUM_SPLITS) in;
layout (triangle_strip, max_vertices = 3) out;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct Partition
{
	float intervalBegin;
	float intervalEnd;

	vec3 scale;
	vec3 bias;
};

struct UPartition
{
	uint intervalBegin;
	uint intervalEnd;

	vec3 scale;
	vec3 bias;
};

struct BoundsUint
{
    uvec3 minCoord;
    uvec3 maxCoord;
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
	BoundsUint gPartitionBoundsU[NUM_FRUSTUM_SPLITS];
};

void main()
{
	if(numDirectionLights > 0)
	{
		vec3 normal = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz);
		vec3 view = -dirLights[0].direction.xyz;
		Partition part = gPartitions[gl_InvocationID];

		if(dot(normal, view) > 0.0f)
		{
			vec4 vertex[3];
			int outOfBound[6] = { 0 , 0 , 0 , 0 , 0 , 0 };
			for (int i =0; i < 3; ++i )
			{
				vertex[i] = dirLights[0].lightSpaceMatrixData.lightProjMatrices[gl_InvocationID] * dirLights[0].lightSpaceMatrixData.lightViewMatrices[gl_InvocationID] * gl_in[i].gl_Position;
				
				if(useSDSMShadows > 0)
				{
					vertex[i].xy = part.scale.xy;
					vertex[i].x += 2.0 * part.bias.x + part.scale.x - 1.0;
					vertex[i].y += 2.0 * part.bias.y + part.scale.y - 1.0;

					vertex[i].z = vertex[i].z * part.scale.z + part.bias.z;
					//vertex[i].z += 2.0 * partition.bias.z + partition.scale.z - 1.0;
				}

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
					gl_Layer = gl_InvocationID;
					EmitVertex();
				}

				EndPrimitive();
			}
		}
	}
}