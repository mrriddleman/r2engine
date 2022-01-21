#version 450 core
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;


const uint MAX_NUM_LIGHTS = 50;
#define NUM_FRUSTUM_SPLITS 4 //TODO(Serge): pass in

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


layout (std140, binding = 0) uniform Matrices 
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
    mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
};

out VS_OUT
{
	vec3 texCoords; 
	vec3 fragPos;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	mat3 TBN;

	vec3 fragPosTangent;
	vec3 viewPosTangent;

	flat uint drawID;

		//section for shadows in the fragment shader - speeds up the shadow calculations
	vec3 fragPosViewSpace;
	vec4 fragPosLightSpace[NUM_FRUSTUM_SPLITS]; //@TODO(Serge): this is only one light...
} vs_out;

void main()
{
	vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	vs_out.fragPos = modelPos.xyz;
	gl_Position = projection * view * modelPos;

	mat3 normalMatrix = transpose(inverse(mat3(models[DrawID])));

	vs_out.normal = normalize(normalMatrix * aNormal);
	vec3 T = normalize(normalMatrix * aTangent);

	T = normalize(T - (dot(T, vs_out.normal) * vs_out.normal));

	vec3 B = normalize(cross(vs_out.normal, T));

	vs_out.tangent = T;
	vs_out.bitangent = B;


	vs_out.TBN = mat3(T, B, vs_out.normal);
	mat3 TBN = transpose(vs_out.TBN);

	vs_out.fragPosTangent = TBN * vs_out.fragPos;
	vs_out.viewPosTangent = TBN * cameraPosTimeW.xyz;

	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	
	if(numDirectionLights > 0)
	{
		vs_out.fragPosViewSpace = vec3(view * vec4(vs_out.fragPos, 1.0));
		vs_out.fragPosLightSpace[0] = dirLights[0].lightSpaceMatrixData.lightProjMatrices[0] * dirLights[0].lightSpaceMatrixData.lightViewMatrices[0] * modelPos; 
		vs_out.fragPosLightSpace[1] = dirLights[0].lightSpaceMatrixData.lightProjMatrices[1] * dirLights[0].lightSpaceMatrixData.lightViewMatrices[1] * modelPos; 
		vs_out.fragPosLightSpace[2] = dirLights[0].lightSpaceMatrixData.lightProjMatrices[2] * dirLights[0].lightSpaceMatrixData.lightViewMatrices[2] * modelPos; 
		vs_out.fragPosLightSpace[3] = dirLights[0].lightSpaceMatrixData.lightProjMatrices[3] * dirLights[0].lightSpaceMatrixData.lightViewMatrices[3] * modelPos; 
	

	}
}