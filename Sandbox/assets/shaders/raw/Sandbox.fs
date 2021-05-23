#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 50;

layout (location = 0) out vec4 FragColor;

#define PI 3.141596

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
};

struct SkyLight
{
	LightProperties lightProperties;
	Tex2DAddress diffuseIrradianceTexture;
	Tex2DAddress prefilteredRoughnessTexture;
	Tex2DAddress lutDFGTexture;
//	int numPrefilteredRoughnessMips;
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
	Tex2DAddress heightTexture1;

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
	SkyLight skylight;

	int numPointLights;
	int numDirectionLights;
	int numSpotLights;
	int numPrefilteredRoughnessMips;
};


in VS_OUT
{
	vec3 texCoords;
	vec3 fragPos;
	vec3 normal;
	mat3 TBN;

	vec3 fragPosTangent;
	vec3 viewPosTangent;

	flat uint drawID;
} fs_in;


vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);
vec4 SampleMaterialNormal(uint drawID, vec3 uv);
vec4 SampleMaterialSpecular(uint drawID, vec3 uv);
vec4 SampleMaterialEmission(uint drawID, vec3 uv);
vec4 SampleMaterialMetallic(uint drawID, vec3 uv);
vec4 SampleMaterialRoughness(uint drawID, vec3 uv);
vec4 SampleMaterialAO(uint drawID, vec3 uv);
vec4 SampleSkylightDiffuseIrradiance(vec3 uv);
vec4 SampleLUTDFG(vec2 uv);
vec4 SampleMaterialPrefilteredRoughness(vec3 uv, float roughnessValue);
float SampleMaterialHeight(uint drawID, vec3 uv);
vec3 ParallaxMapping(uint drawID, vec3 uv, vec3 viewDir);


float Fd_Lambert();
float Fd_Burley(float roughness, float NoV, float NoL, float LoH);

float D_GGX(float NoH, float roughness);
vec3  F_Schlick(float LoH, vec3 F0);
float F_Schlick(float f0, float f90, float VoH);
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness);
vec3 BRDF(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoL, float roughness);

vec3 CalculateLightingBRDF(vec3 N, vec3 V, vec3 baseColor, uint drawID, vec3 uv);

float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

void main()
{
	vec3 viewDir = normalize(cameraPosTimeW.xyz - fs_in.fragPos);

	vec3 texCoords = fs_in.texCoords;

	vec3 viewDirTangent = normalize(fs_in.viewPosTangent - fs_in.fragPosTangent);

	texCoords = ParallaxMapping(fs_in.drawID, texCoords, viewDirTangent);

	if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, texCoords);
	vec3 norm = SampleMaterialNormal(fs_in.drawID, texCoords).rgb;

	vec3 lightingResult = CalculateLightingBRDF(norm, viewDir, sampledColor.rgb, fs_in.drawID, texCoords);


	vec3 emission = SampleMaterialEmission(fs_in.drawID, texCoords).rgb;

	FragColor = vec4(lightingResult + emission , 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].baseColor,1) + modifier * textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
}

vec4 SampleMaterialNormal(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;

	Tex2DAddress addr = materials[texIndex].normalMapTexture1;

	vec3 coord = vec3(uv.rg, addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	vec3 normalMapNormal = textureLod(sampler2DArray(addr.container), coord, mipmapLevel).rgb;

	normalMapNormal = normalMapNormal * 2.0 - 1.0;

	normalMapNormal = normalize(fs_in.TBN * normalMapNormal);

	return (1.0 - modifier) * vec4(fs_in.normal, 1) +  modifier * vec4(normalMapNormal, 1);
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

	return (1.0 - modifier) * vec4(0.2) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page));
}

vec4 SampleMaterialPrefilteredRoughness(vec3 uv, float roughnessValue)
{
	Tex2DAddress addr = skylight.prefilteredRoughnessTexture;
	return textureLod(samplerCubeArray(addr.container), vec4(uv, addr.page), roughnessValue);
}


float SampleMaterialHeight(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].heightTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * 0.0 + modifier * (1.0 - texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page)).r);
}

vec3 ParallaxMapping(uint drawID, vec3 uv, vec3 viewDir)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].heightTexture1;

	float modifier = GetTextureModifier(addr);

	if(modifier <= 0.0)
		return uv;

	float currentLayerDepth = 0.0;

	const float minLayers = 8;
	const float maxLayers = 32;
	const float heightScale = 0.05;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

	float layerDepth = 1.0 / numLayers;

	vec2 P = viewDir.xy / viewDir.z * heightScale;

	vec2 deltaTexCoords = P / numLayers;

	vec2 currentTexCoords = uv.rg;
	float currentDepthMapValue = SampleMaterialHeight(drawID, uv);

	while(currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;

		currentDepthMapValue = SampleMaterialHeight(drawID, vec3(currentTexCoords, uv.b));

		currentLayerDepth += layerDepth;
	}

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	float afterDepth = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = SampleMaterialHeight(drawID, vec3(prevTexCoords, uv.b)) - currentLayerDepth + layerDepth;

	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
	
	return vec3(finalTexCoords, uv.b);
	
}

vec4 SampleLUTDFG(vec2 uv)
{
	Tex2DAddress addr = skylight.lutDFGTexture;
	return texture(sampler2DArray(addr.container), vec3(uv, addr.page));
}

vec4 SampleSkylightDiffuseIrradiance(vec3 uv)
{
	Tex2DAddress addr = skylight.diffuseIrradianceTexture;
	return texture(samplerCubeArray(addr.container), vec4(uv, addr.page));
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

float F_Schlick(float f0, float f90, float VoH)
{
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
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

float Fd_Burley(float roughness, float NoV, float NoL, float LoH) 
{
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(1.0, f90, NoL);
    float viewScatter  = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}


vec3 BRDF(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoL, float roughness)
{
	vec3 H = normalize(V + L);

	float NoV = abs(dot(N, V)) + 1e-5;
	float NoH = clamp(dot(N, H), 0.0, 1.0);
	float LoH = clamp(dot(L, H), 0.0, 1.0);

	float D = D_GGX(NoH, roughness);
	vec3 F = F_Schlick(max(dot(H, V), 0.0), F0);
	float VSmith = V_SmithGGXCorrelated(NoV, NoL, roughness);

	//specular BRDF
	vec3 Fr = (D * VSmith) * F;

	//Energy compensation
	//vec3 energyCompensation = 1.0 + F0 * (1.0 / dfg.y - 1.0);
	//Fr *= pixel.energyCompensation

	//float denom = 4.0 * NoV * NoL;
	vec3 specular = Fr;// max(denom, 0.001);
	vec3 Fd = diffuseColor * Fd_Lambert(); //Fd_Burley(roughness, NoV, NoL, LoH);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;

	return (kD * Fd + specular);
}


float GetDistanceAttenuation(vec3 posToLight, float falloff)
{
	float distanceSquare = dot(posToLight, posToLight);

    float factor = distanceSquare * falloff;

    float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);

    float attenuation = smoothFactor * smoothFactor;

    return attenuation * 1.0 / max(distanceSquare, 1e-4);
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

	vec3 R = reflect(-V, N);

	vec3 diffuseIrradiance = SampleSkylightDiffuseIrradiance(N).rgb;

	vec3 L0 = vec3(0,0,0);

	 for(int i = 0; i < numDirectionLights; i++)
	 {
	 	DirLight dirLight = dirLights[i];

	 	vec3 L = normalize(-dirLight.direction.xyz);

	 	float NoL = clamp(dot(N, L), 0.0, 1.0);

	 	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity;

	 	vec3 result = BRDF(diffuseColor, N, V, L, F0, NoL, roughness);

	 	L0 += result * radiance * NoL;
	 }

	for(int i = 0; i < numPointLights; ++i)
	{
		PointLight pointLight = pointLights[i];

		vec3 posToLight = pointLight.position.xyz - fs_in.fragPos;

		vec3 L = normalize(posToLight);

		float NoL = clamp(dot(N, L), 0.0, 1.0);

		float attenuation = GetDistanceAttenuation(posToLight, pointLight.lightProperties.fallOffRadius);

		vec3 radiance = pointLight.lightProperties.color.rgb * attenuation * pointLight.lightProperties.intensity * exposure.x;

		vec3 result = BRDF(diffuseColor, N, V, L, F0, NoL, roughness);
		
		L0 += result * radiance * NoL;
	}

	for(int i = 0; i < numSpotLights; ++i)
	{
		SpotLight spotLight = spotLights[i];

		vec3 posToLight = spotLight.position.xyz - fs_in.fragPos;

		vec3 L = normalize(posToLight);

		float NoL = clamp(dot(N, L), 0.0, 1.0);

		float attenuation = GetDistanceAttenuation(posToLight, spotLight.lightProperties.fallOffRadius);

		//calculate the spot angle attenuation

		float theta = dot(L, normalize(-spotLight.direction.xyz));

		float epsilon = max(spotLight.position.w - spotLight.direction.w, 1e-4);

		float spotAngleAttenuation = clamp((theta - spotLight.direction.w) / epsilon, 0.0, 1.0);

		spotAngleAttenuation = spotAngleAttenuation * spotAngleAttenuation;


		vec3 radiance = spotLight.lightProperties.color.rgb * attenuation * spotAngleAttenuation * spotLight.lightProperties.intensity * exposure.x;

		vec3 result = BRDF(diffuseColor, N, V, L, F0, NoL, roughness);

		L0 += result * radiance * NoL;
	}

	vec3 kS = F_Schlick(max(dot(N, V), 0.0), F0);
	vec3 kD = 1.0 - kS;

	vec3 prefilteredColor = SampleMaterialPrefilteredRoughness(R, roughness * numPrefilteredRoughnessMips).rgb;
	vec2 brdf = SampleLUTDFG(vec2(max(dot(N,V), 0.0), roughness)).rg;
	vec3 specular = prefilteredColor * (kS * brdf.x + kS * brdf.y);

	vec3 ambient = (( diffuseColor * diffuseIrradiance * Fd_Lambert()) + specular) * ao;

	
	vec3 color =  ambient + L0;

	return color;
}