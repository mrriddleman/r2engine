#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 1000;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct AttenuationState
{
    float constant;
    float linear;
    float quadratic;
};

struct Light
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
};

struct LightProperties
{
	Light lightModifiers;
	vec3 color;
	float specular;
	float strength;
	AttenuationState attenuation;
};

struct PointLight
{
	LightProperties lightProperties;
	vec3 position;
};

struct DirLight
{
	LightProperties lightProperties;
	vec3 direction;
};

struct SpotLight
{
	LightProperties lightProperties;
	vec3 position;
	vec3 direction;
	float radius;
	float cutoff;
};

struct Material
{
	Tex2DAddress diffuseTexture1;
	Tex2DAddress specularTexture1;
	Tex2DAddress normalMapTexture1;
	Tex2DAddress emissionTexture1;

	vec4 baseColor;
	float specular;
	float roughness;
	float metallic;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

layout (std430, binding = 4) buffer Lighting
{

	PointLight pointLights[MAX_NUM_LIGHTS];
	DirLight dirLights[MAX_NUM_LIGHTS];
	SpotLight spotLights[MAX_NUM_LIGHTS];

	uint numPointLights;
	uint numDirectionLights;
	uint numSpotLights;
};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);
vec4 SampleMaterialSpecular(uint drawID, vec3 uv);
vec4 SampleMaterialEmission(uint drawID, vec3 uv);

vec3 CalcPointLight(uint pointLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcDirLight(uint dirLightIndex, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(uint spotLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir);

vec3 HalfVector(vec3 lightDir, vec3 viewDir);
float CalcSpecular(vec3 lightDir, vec3 viewDir, float shininess);
float PhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess);
float BlinnPhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess);

Light CalcLightForMaterial(Light light, float diffuse, float specular, float modifier);

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	FragColor = vec4(sampledColor.rgb, 1.0);
	if(sampledColor.a < 0.01)
		discard;
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;
	return texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}

vec4 SampleMaterialSpecular(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].specularTexture1;
	return texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}

vec4 SampleMaterialEmission(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].emissionTexture1;
	return texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}

float CalcAttenuation(AttenuationState state, vec3 lightPos, vec3 fragPos)
{
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (state.constant + state.linear * distance + state.quadratic * (distance * distance));
    return attenuation;
}

vec3 CalcPointLight(uint pointLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	PointLight pointLight = pointLights[pointLightIndex];

	vec3 lightDir = normalize(pointLight.position - fragPos);

	float diffuse = max(dot(normal, lightDir), 0.0);

	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	float attenuation = CalcAttenuation(pointLight.lightProperties.attenuation, pointLight.position, fragPos);

	Light result = CalcLightForMaterial(pointLight.lightProperties.lightModifiers, diffuse, specular, attenuation);

	return (result.ambient + result.diffuse + result.specular) * pointLight.lightProperties.color;
}

vec3 CalcDirLight(uint dirLightIndex, vec3 normal, vec3 viewDir)
{
	DirLight dirLight = dirLights[dirLightIndex];

	vec3 lightDir = normalize(-dirLight.direction);

	float diffuse = max(dot(normal, lightDir), 0.0);

	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	Light result = CalcLightForMaterial(dirLight.lightProperties.lightModifiers, diffuse, specular, 1.0);

	return (result.ambient + result.diffuse + result.specular) * dirLight.lightProperties.color;
}

vec3 CalcSpotLight(uint spotLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	SpotLight spotLight = spotLights[spotLightIndex];


	vec3 lightDir = normalize(spotLight.position - fragPos);

	float diffuse = max(dot(normal, lightDir), 0.0);

	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	float theta = dot(lightDir, normalize(spotLight.direction));

	float epsilon = spotLight.radius - spotLight.cutoff;
	float intensity = clamp((theta - spotLight.cutoff) / epsilon, 0.0, 1.0);

	float attenuation = CalcAttenuation(spotLight.lightProperties.attenuation, spotLight.position, fragPos);

	Light result = CalcLightForMaterial(spotLight.lightProperties.lightModifiers, diffuse, specular, attenuation * intensity);

	return (result.ambient + result.diffuse + result.specular) * spotLight.lightProperties.color;
}

Light CalcLightForMaterial(Light light, float diffuse, float specular, float modifier)
{
	Light result;

	vec3 diffuseMat = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords).rgb;
	result.ambient = light.ambient * diffuseMat;

	result.diffuse = light.diffuse * diffuse * diffuseMat;
	result.specular = light.specular * specular * SampleMaterialSpecular(fs_in.drawID, fs_in.texCoords).rgb;

	result.ambient *= modifier;
	result.diffuse *= modifier;
	result.specular *= modifier;

	return result;
}

vec3 CalcEmissionForMaterial(Light light)
{
	vec3 emission = light.emission * SampleMaterialEmission(fs_in.drawID, fs_in.texCoords).rgb;
	return emission;
}

vec3 HalfVector(vec3 lightDir, vec3 viewDir)
{
    return normalize(normalize(lightDir) + normalize(viewDir));
}

float CalcSpecular(vec3 lightDir, vec3 viewDir, float shininess)
{   
    return pow(max(dot(viewDir, lightDir), 0.0), shininess);
}

float PhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess)
{
    vec3 reflectDir = reflect(-inLightDir, normal);
    return CalcSpecular(reflectDir, viewDir, shininess);
}

float BlinnPhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess)
{
    return CalcSpecular(HalfVector(inLightDir, viewDir), normal, shininess);
}
