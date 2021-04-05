#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 100;
#define PI 3.141596

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
	vec4 position; // w is radius
	vec4 direction; //w is cutoff
	//float radius;
	//float cutoff;
};

struct Material
{
	Tex2DAddress diffuseTexture1;
	Tex2DAddress specularTexture1;
	Tex2DAddress normalMapTexture1;
	Tex2DAddress emissionTexture1;
	Tex2DAddress metallicTexture1;
	Tex2DAddress roughnessTexture1;
	Tex2DAddress aoTexture1;

	vec3 baseColor;
	float specular;
	float roughness;
	float metallic;
	float reflectance;
	float ambientOcclusion;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposure;
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

	mat3 TBN;

	flat uint drawID;

} fs_in;


vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);
vec4 SampleMaterialSpecular(uint drawID, vec3 uv);
vec4 SampleMaterialNormal(uint drawID, vec3 uv);
vec4 SampleMaterialEmission(uint drawID, vec3 uv);
vec4 SampleMaterialMetallic(uint drawID, vec3 uv);
vec4 SampleMaterialRoughness(uint drawID, vec3 uv);
vec4 SampleMaterialAO(uint drawID, vec3 uv);

vec3 CalcPointLight(uint pointLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcDirLight(uint dirLightIndex, vec3 normal, vec3 viewDir);
vec3 CalcSpotLight(uint spotLightIndex, vec3 normal, vec3 fragPos, vec3 viewDir);

vec3 HalfVector(vec3 lightDir, vec3 viewDir);
float CalcSpecular(vec3 lightDir, vec3 viewDir, float shininess);
float PhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess);
float BlinnPhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess);

Light CalcLightForMaterial(float diffuse, float specular, float modifier);

float Fd_Lambert();
float D_GGX(float NoH, float roughness);
vec3  F_Schlick(float LoH, vec3 F0);
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness);
vec3 BRDF(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoL, float roughness);

vec3 CalculateLightingBRDF(vec3 N, vec3 V, vec3 baseColor, uint drawID, vec3 uv);

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);
	if(sampledColor.a < 0.01)
		discard;

	vec3 norm = normalize(SampleMaterialNormal(fs_in.drawID, fs_in.texCoords).rgb);

	vec3 viewDir = normalize(cameraPosTimeW.rgb - fs_in.fragPos);

	vec3 lightingResult = CalculateLightingBRDF(norm, viewDir, sampledColor.rgb, fs_in.drawID, fs_in.texCoords);

	

	// for(int i = 0; i < numDirectionLights; i++)
	// {
	// 	lightingResult += CalcDirLight(i, norm, viewDir);
	// }

	// for(int i = 0; i < numPointLights; ++i)
	// {
	// 	lightingResult += CalcPointLight(i, norm, fs_in.fragPos, viewDir);
	// }

	// for(int i = 0; i < numSpotLights; ++i)
	// {
	// 	lightingResult += CalcSpotLight(i, norm, fs_in.fragPos, viewDir);
	// }


	vec3 emission = SampleMaterialEmission(fs_in.drawID, fs_in.texCoords).rgb;

	FragColor = vec4(lightingResult + emission, 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].baseColor, 1) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}

vec4 SampleMaterialSpecular(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].specularTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(vec3(materials[texIndex].specular), 1.0) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}

vec4 SampleMaterialNormal(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
 
	Tex2DAddress addr = materials[texIndex].normalMapTexture1;

	float modifier = GetTextureModifier(addr);

	vec3 coord = vec3(uv.rg, addr.page);

	vec3 normalMapNormal = texture(sampler2DArray(addr.container), coord).rgb;

	normalMapNormal = normalMapNormal * 2.0 - 1.0;

	normalMapNormal = normalize(fs_in.TBN * normalMapNormal);

	return (1.0 - modifier) * vec4(fs_in.normal, 1) + modifier * vec4(normalMapNormal, 1);
}

vec4 SampleMaterialEmission(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].emissionTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(0.0) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}

vec4 SampleMaterialMetallic(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].metallicTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].metallic) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page));
}

vec4 SampleMaterialRoughness(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].roughnessTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].roughness) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page));
}

vec4 SampleMaterialAO(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].aoTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].ambientOcclusion) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page));
}

float CalcAttenuation(vec3 state, vec3 lightPos, vec3 fragPos)
{
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (distance * distance);
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

	return (result.ambient + result.diffuse + result.specular) * pointLight.lightProperties.color.rgb * pointLight.lightProperties.strength;
}

vec3 CalcDirLight(uint dirLightIndex, vec3 normal, vec3 viewDir)
{

	DirLight dirLight = dirLights[dirLightIndex];

	vec3 lightDir = normalize(-dirLight.direction.xyz);

	float diffuse = max(dot(normal, lightDir), 0.0);

	float specular = BlinnPhongShading(lightDir, viewDir, normal, 0.3);

	Light result = CalcLightForMaterial(diffuse, specular, 1.0);

	return (result.ambient + result.diffuse + result.specular) * dirLight.lightProperties.color.rgb * dirLight.lightProperties.strength;
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

	return (result.ambient + result.diffuse + result.specular) * spotLight.lightProperties.color.rgb * spotLight.lightProperties.strength;
}

Light CalcLightForMaterial(float diffuse, float specular, float modifier)
{
	Light result;

	vec3 diffuseMat = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords).rgb;
	result.ambient =  diffuseMat;

	result.diffuse =  diffuse * diffuseMat;
	result.specular = specular * SampleMaterialSpecular(fs_in.drawID, fs_in.texCoords).rgb;

	result.ambient *= modifier;
	result.diffuse *= modifier;
	result.specular *= modifier;

	return result;
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


float D_GGX(float NoH, float roughness)
{
	float a2 = roughness * roughness;
	float f = (NoH * a2 - NoH) * NoH + 1.0;
	return a2 / (PI * f * f);
}

vec3 F_Schlick(float LoH, vec3 F0)
{
	return F0 + (vec3(1.0) - F0) * pow(1.0 - LoH, 5.0);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
	float a2 = roughness * roughness;
	float GGXL = NoV * sqrt((-NoL * a2 + NoL) * NoL + a2);
	float GGXV = NoL * sqrt((-NoV * a2 + NoV) * NoV + a2);
	return clamp(0.5 / (GGXV + GGXL), 0.0, 1.0);
}

float Fd_Lambert()
{
	return 1.0 / PI;
}

vec3 BRDF(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoL, float roughness)
{
	vec3 H = normalize(V + L);

	float NoV = abs(dot(N, V)) + 1e-5;
	float NoH = clamp(dot(N, H), 0.0, 1.0);
	float LoH = clamp(dot(L, H), 0.0, 1.0);

	float D = D_GGX(NoH, roughness);
	vec3 F = F_Schlick(LoH, F0);
	float VSmith = V_SmithGGXCorrelated(NoV, NoL, roughness);

	//specular BRDF
	vec3 Fr = (D * VSmith) * F;

	//Energy compensation
	//vec3 energyCompensation = 1.0 + F0 * (1.0 / dfg.y - 1.0);
	//Fr *= pixel.energyCompensation

	//float denom = 4.0 * NoV * NoL;
	vec3 specular = Fr;// max(denom, 0.001);
	vec3 Fd = diffuseColor * Fd_Lambert();

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;

	return (kD * Fd + specular);
}


vec3 CalculateLightingBRDF(vec3 N, vec3 V, vec3 baseColor, uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	
	float reflectance = materials[texIndex].reflectance;

	float metallic = SampleMaterialMetallic(drawID, uv).r;

	float ao = SampleMaterialAO(drawID, uv).r;

	float perceptualRoughness = SampleMaterialRoughness(drawID, uv).r;

	float roughness = perceptualRoughness * perceptualRoughness;

	vec3 F0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;

	vec3 diffuseColor = (1.0 - metallic) * baseColor;


	vec3 L0 = vec3(0,0,0);

	 for(int i = 0; i < numDirectionLights; i++)
	 {
	 	DirLight dirLight = dirLights[i];

	 	vec3 L = normalize(-dirLight.direction.xyz);

	 	float NoL = clamp(dot(N, L), 0.0, 1.0);

	 	vec3 radiance = dirLight.lightProperties.color.rgb;

	 	vec3 result = BRDF(diffuseColor, N, V, L, F0, NoL, roughness);

	 	L0 += result * radiance * NoL;
	 }

	for(int i = 0; i < numPointLights; ++i)
	{
		PointLight pointLight = pointLights[i];

		vec3 L = pointLight.position.xyz - fs_in.fragPos;

		float distance = length(L);  

		L = normalize(L);

		float NoL = clamp(dot(N, L), 0.0, 1.0);

		float attenuation = 1.0 / (distance * distance);

		vec3 radiance = pointLight.lightProperties.color.rgb * attenuation;

		vec3 result = BRDF(diffuseColor, N, V, L, F0, NoL, roughness);
		
		L0 += result * radiance * NoL;
	}

	for(int i = 0; i < numSpotLights; ++i)
	{
		SpotLight spotLight = spotLights[i];

		vec3 L = spotLight.position.xyz - fs_in.fragPos;

		float distance = length(L);

		L = normalize(L);

		float NoL = clamp(dot(N, L), 0.0, 1.0);

		float attenuation = 1.0 / (distance * distance);

		float theta = dot(L, normalize(-spotLight.direction.xyz));

		float epsilon = spotLight.position.w - spotLight.direction.w;

		float intensity = clamp((theta - spotLight.direction.w) / epsilon, 0.0, 1.0);

		vec3 radiance = spotLight.lightProperties.color.rgb * attenuation * intensity;

		vec3 result = BRDF(diffuseColor, N, V, L, F0, NoL, roughness);

		L0 += result * radiance * NoL;
	}

	vec3 ambient = vec3(0.03) * baseColor * ao;
	vec3 color = ambient + L0;

	return color;
}