#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 100;

layout (location = 0) out vec4 FragColor;

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
	vec4 color;
	vec4 attenuation;
	float specular;
	float strength;
	int64_t lightID;
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
};

struct SpotLight
{
	LightProperties lightProperties;
	vec4 position;//w is radius
	vec4 direction;//w is cutoff
	//float radius;
	//float cutoff;
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

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int temp;
};


in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	vec3 fragPos;
	flat uint drawID;
} fs_in;

const float PI = 3.14159;
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

Light CalcLightForMaterial(float diffuse, float specular, float modifier);

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

void main()
{
	vec3 norm = normalize(fs_in.normal);
	vec3 viewDir = normalize(cameraPosTimeW.xyz - fs_in.fragPos);

	vec3 lightingResult = vec3(0,0,0);

	for(int i = 0; i < numDirectionLights; i++)
	{
		lightingResult += CalcDirLight(i, norm, viewDir);
	}

	for(int i = 0; i < numPointLights; ++i)
	{
		lightingResult += CalcPointLight(i, norm, fs_in.fragPos, viewDir);
	}

	for(int i = 0; i < numSpotLights; ++i)
	{
		lightingResult += CalcSpotLight(i, norm, fs_in.fragPos, viewDir);
	}

	FragColor = vec4(lightingResult, 1.0);
	//vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	//FragColor = vec4(sampledColor.rgb, 1.0);// * (0.4 * sin(cameraPosTimeW.w+PI*1.5) + 0.6);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * materials[texIndex].baseColor + modifier * textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
}

vec4 SampleMaterialSpecular(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].specularTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(vec3(materials[texIndex].specular), 1.0) + modifier * textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
}

vec4 SampleMaterialEmission(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].emissionTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(0.0) + modifier * textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
}


float CalcAttenuation(vec3 state, vec3 lightPos, vec3 fragPos)
{
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (state.x + state.y * distance + state.z * (distance * distance));
    return attenuation;
}

vec3 CalcPointLight(uint pointLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	PointLight pointLight = pointLights[pointLightIndex];

	vec3 lightDir = normalize(pointLight.position.xyz - fragPos);

	float diffuse = max(dot(normal, lightDir), 0.0);
 
	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	float attenuation = CalcAttenuation(pointLight.lightProperties.attenuation.xyz, pointLight.position.xyz, fragPos);

	Light result = CalcLightForMaterial(diffuse, specular, attenuation);

	return (result.ambient + result.diffuse + result.specular + result.emission) * pointLight.lightProperties.color.rgb * pointLight.lightProperties.strength;
}

vec3 CalcDirLight(uint dirLightIndex, vec3 normal, vec3 viewDir)
{
	
	DirLight dirLight = dirLights[dirLightIndex];

	vec3 lightDir = normalize(-dirLight.direction.xyz);

	float diffuse = max(dot(normal, lightDir), 0.0);

	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	Light result = CalcLightForMaterial(diffuse, specular, 1.0);

	return (result.ambient + result.diffuse + result.specular + result.emission) * dirLight.lightProperties.color.rgb * dirLight.lightProperties.strength;
}

vec3 CalcSpotLight(uint spotLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	SpotLight spotLight = spotLights[spotLightIndex];

	vec3 lightDir = normalize(spotLight.position.xyz - fragPos);

	float diffuse = max(dot(normal, lightDir), 0.0);

	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	float theta = dot(lightDir, normalize(-spotLight.direction.xyz));

	float epsilon = spotLight.position.w - spotLight.direction.w;
	float intensity = clamp((theta - spotLight.direction.w) / epsilon, 0.0, 1.0);

	float attenuation = CalcAttenuation(spotLight.lightProperties.attenuation.xyz, spotLight.position.xyz, fragPos);

	Light result = CalcLightForMaterial(diffuse, specular, attenuation * intensity);

	return (result.ambient + result.diffuse + result.specular + result.emission) * spotLight.lightProperties.color.rgb * spotLight.lightProperties.strength;
}

Light CalcLightForMaterial(float diffuse, float specular, float modifier)
{
	Light result;

	vec3 diffuseMat = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords).rgb;
	result.ambient =  diffuseMat;

	result.diffuse =  diffuse * diffuseMat;
	result.specular =  specular * SampleMaterialSpecular(fs_in.drawID, fs_in.texCoords).rgb;
	result.emission =  SampleMaterialEmission(fs_in.drawID, fs_in.texCoords).rgb;

	result.ambient *= modifier;
	result.diffuse *= modifier;
	result.specular *= modifier;
	result.emission *= modifier;

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