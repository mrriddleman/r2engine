#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint MAX_NUM_LIGHTS = 50;

const float NUM_SOFT_SHADOW_SAMPLES = 16.0;
const float SHADOW_FILTER_MAX_SIZE = 0.0025f;
const float PENUMBRA_FILTER_SCALE = 2.4f;
const uint NUM_SIDES_FOR_POINTLIGHT = 6;

layout (location = 0) out vec4 FragColor;

#define PI 3.141596
#define TWO_PI 2.0 * PI
#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define NUM_FRUSTUM_SPLITS 4 //TODO(Serge): pass in

#define NUM_SPOTLIGHT_LAYERS 1
#define NUM_POINTLIGHT_LAYERS 6
#define NUM_DIRECTIONLIGHT_LAYERS NUM_FRUSTUM_SPLITS

#define NUM_SPOTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_POINTLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS
#define NUM_DIRECTIONLIGHT_SHADOW_PAGES MAX_NUM_LIGHTS



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

	mat4 lightSpaceMatrices[NUM_SIDES_FOR_POINTLIGHT];
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

struct RenderMaterialParam
{
	Tex2DAddress texture;
	vec4 color;
};

struct Material
{
	RenderMaterialParam albedo;
	RenderMaterialParam normalMap;
	RenderMaterialParam emission;
	RenderMaterialParam metallic;
	RenderMaterialParam roughness;
	RenderMaterialParam ao;
	RenderMaterialParam height;
	RenderMaterialParam anisotropy;
	RenderMaterialParam detail;

	RenderMaterialParam clearCoat;
	RenderMaterialParam clearCoatRoughness;
	RenderMaterialParam clearCoatNormal;

	int 	doubleSided;
	float 	heightScale;
	float	reflectance;
	int 	padding;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposureNearFar;
    vec4 cascadePlanes;
    vec4 shadowMapSizes;
    vec4 fovAspectResXResY;
    uint64_t frame;
    uint64_t unused;
};


layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

layout (std430, binding = 7) buffer MaterialOffsets
{
	uint materialOffsets[];
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
    mat4 inverseProjection;
};

//@NOTE(Serge): this is in the order of the render target surfaces in RenderTarget.h
layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
};

//@NOTE(Serge): we can only have 4 cascades like this
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


in VS_OUT
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

} fs_in;

vec4 splitColors[NUM_FRUSTUM_SPLITS] = {vec4(2, 0.0, 0.0, 1.0), vec4(0.0, 2, 0.0, 1.0), vec4(0.0, 0.0, 2, 1.0), vec4(2, 2, 0.0, 1.0)};

vec2 VogelDiskSample(int sampleIndex, int samplesCount, float phi);
float InterleavedGradientNoise(vec2 screenPosition);
float Penumbra(float gradientNoise, vec2 shadowMapUV, float depth, int samplesCount, vec2 shadowMapSizeInv, uint cascadeIndex, int64_t lightID);
float AvgBlockersDepthToPenumbra(float depth, float avgBlockersDepth);

float OptimizedPCF(vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float lightDepth);
float SoftShadow(vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float lightDepth);
float ShadowCalculation(vec3 fragPosWorldSpace, vec3 lightDir, int64_t lightID, bool softShadows);
float SampleShadowCascade(vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float NoL, bool softShadows);

float SpotLightShadowCalculation(vec3 fragPosWorldSpace, vec3 lightDir, int64_t lightID, int lightIndex, bool softShadows);

float PointLightShadowCalculation(vec3 fragToLight, vec3 viewPos, float farPlane, int64_t lightID, bool softShadows);

float SampleDirectionShadowMap(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, uint cascadeIndex, int64_t lightID, float depth);
float SampleSpotlightShadowMap(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, int64_t spotlightID, float depth);
float SamplePointlightShadowMap(vec3 fragToLight, vec3 offset, int64_t lightID);

vec3 CalculateClearCoatBaseF0(vec3 F0, float clearCoat);

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);
vec4 SampleMaterialNormal(uint drawID, vec3 uv);
vec4 SampleMaterialEmission(uint drawID, vec3 uv);
vec4 SampleMaterialMetallic(uint drawID, vec3 uv);
vec4 SampleMaterialRoughness(uint drawID, vec3 uv);
vec4 SampleMaterialAO(uint drawID, vec3 uv);
vec4 SampleDetail(uint drawID, vec3 uv);
vec4 SampleClearCoat(uint drawID, vec3 uv);
vec4 SampleClearCoatRoughness(uint drawID, vec3 uv);

vec4 SampleSkylightDiffuseIrradiance(vec3 uv);
vec4 SampleLUTDFG(vec2 uv);
vec4 SampleMaterialPrefilteredRoughness(vec3 uv, float roughnessValue);
float SampleMaterialHeight(uint drawID, vec3 uv);
vec3 ParallaxMapping(uint drawID, vec3 uv, vec3 viewDir);
vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness);

float Fd_Lambert();
float Fd_Burley(float roughness, float NoV, float NoL, float LoH);

float D_GGX(float NoH, float roughness);
float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH);
vec3  F_Schlick(float LoH, vec3 F0);
float F_Schlick(float f0, float f90, float VoH);
vec3 F_Schlick(const vec3 F0, float F90, float VoH);
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness);
float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV, float ToL, float BoL, float NoV, float NoL);
float V_Kelemen(float LoH);
float ClearCoatLobe(float clearCoat, float clearCoatRoughness, float NoH, float LoH, out float Fcc);


vec3 BRDF(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoV, float NoL, float ggxVTerm, float roughness, float clearCoat, float clearCoatRoughness, float shadow);
vec3 BRDF_Anisotropic(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoV, float NoL, float ggxVTerm, float roughness, float shadow);

vec3 Eval_BRDF(
	float anisotropy,
	float at,
	float ab,
	vec3 anisotropicT,
	vec3 anisotropicB,
	vec3 diffuseColor,
	vec3 N,
	vec3 V,
	vec3 L,
	vec3 F0,
	float NoV,
	float ToV,
	float BoV,
	float NoL,
	float ggxVTerm,
	vec3 energyCompensation,
	float roughness,
	float clearCoat,
	float clearCoatRoughness,
	vec3 clearCoatNormal,
	float shadow);


vec3 CalculateLightingBRDF(vec3 N, vec3 V, vec3 baseColor, uint drawID, vec3 uv);



float GetTextureModifier(Tex2DAddress addr)
{
	return float( min(max(addr.container, 0), 1) );
}

float Saturate(float x)
{
	return clamp(x, 0.0, 1.0);
}

vec4 DebugFrustumSplitColor()
{
	if(numDirectionLights > 0)
	{
		vec4 projectionPosInCSMSplitSpace = (gShadowMatrix[0] * vec4(fs_in.fragPos, 1.0));

		vec3 projectionPos = projectionPosInCSMSplitSpace.xyz;

		uint layer = NUM_FRUSTUM_SPLITS - 1;



		for(int i = int(layer); i >= 0; --i)
		{
			vec3 scale = gScale[i][0].xyz;
			vec3 bias = gBias[i][0].xyz;

			vec3 cascadePos = projectionPos + bias;
			cascadePos *= scale;
			cascadePos = abs(cascadePos - 0.5f);
			if(cascadePos.x <= 0.5 && cascadePos.y <= 0.5 && cascadePos.z <= 0.5)
			{
				layer = i;
			}
		}

		if(layer == -1)
		{
			layer = NUM_FRUSTUM_SPLITS-1;
		}

		return splitColors[layer];
	}
	

	return vec4(1);
}


void main()
{

	//vec4 debugCascadePlanes = {1000.0 / 100.0, 1000.0 / 20.0, 1000.0 / 10.0, 1000.0};

	vec3 viewDir = normalize(cameraPosTimeW.xyz - fs_in.fragPos);

	vec3 texCoords = fs_in.texCoords;

	vec3 viewDirTangent = normalize(fs_in.viewPosTangent - fs_in.fragPosTangent);

	//texCoords = ParallaxMapping(fs_in.drawID, texCoords, viewDirTangent);

	//if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
 	//    discard;

	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, texCoords);



	vec3 norm = SampleMaterialNormal(fs_in.drawID, texCoords).rgb;

	vec3 lightingResult = CalculateLightingBRDF(norm, viewDir, sampledColor.rgb, fs_in.drawID, texCoords);


	vec3 emission = SampleMaterialEmission(fs_in.drawID, texCoords).rgb;



	
	

	//vec3 fragToLight = (fs_in.fragPos - pointLights[0].position.xyz);
//	fragToLight = vec3(fragToLight.x, fragToLight.z, -fragToLight.y);
	//vec4 coord = vec4(fragToLight, gPointLightShadowMapPages[int(pointLights[0].lightProperties.lightID)] );

	//FragColor = vec4(vec3(texture(samplerCubeArray(pointLightShadowsSurface.container), coord).r/25.0), 1);
	//FragColor = vec4(texCoords.x, texCoords.y, 0, 1.0);
	FragColor = vec4(lightingResult + emission , 1.0);// * DebugFrustumSplitColor();

}

vec4 SampleTexture(Tex2DAddress addr, vec3 coord, float mipmapLevel)
{
	vec4 textureSample = textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
	return addr.channel < 0 ? vec4(textureSample.rgba) : vec4(textureSample[addr.channel]); //no rgb right now
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].albedo.texture;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].albedo.color.rgb,1) + modifier * SampleTexture(addr, coord, mipmapLevel);
}

vec4 SampleMaterialNormal(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];

	Tex2DAddress addr = materials[texIndex].normalMap.texture;

	vec3 coord = vec3(uv.rg, addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	vec3 normalMapNormal = SampleTexture(addr, coord, mipmapLevel).rgb;

	normalMapNormal = normalize(normalMapNormal * 2.0 - 1.0);

	normalMapNormal = normalize(fs_in.TBN * normalMapNormal);

	return  (1.0 - modifier) * vec4(fs_in.normal, 1) +  modifier * vec4(normalMapNormal, 1);
}

vec4 SampleMaterialEmission(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].emission.texture;

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(materials[texIndex].emission.color.rgb,1) + modifier * SampleTexture(addr, coord, mipmapLevel);
}

vec4 SampleMaterialMetallic(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].metallic.texture;
	//addr.channel = 2;

	float modifier = GetTextureModifier(addr);

	vec4 color = materials[texIndex].metallic.color;

	return (1.0 - modifier) * color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialRoughness(uint drawID, vec3 uv)
{
	//@TODO(Serge): put this back to roughnessTexture1
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].roughness.texture;

	float modifier = GetTextureModifier(addr);

	vec4 color = materials[texIndex].roughness.color;

	//@TODO(Serge): put this back to not using the alpha
	return (1.0 - modifier) * color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialAO(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].ao.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(1.0) + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleDetail(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].detail.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * materials[texIndex].detail.color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleClearCoat(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].clearCoat.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * materials[texIndex].clearCoat.color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleClearCoatRoughness(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].clearCoatRoughness.texture;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * materials[texIndex].clearCoatRoughness.color + modifier * SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0);
}

vec4 SampleMaterialPrefilteredRoughness(vec3 uv, float roughnessValue)
{
	Tex2DAddress addr = skylight.prefilteredRoughnessTexture;
	//z is up so we rotate this... not sure if this is right?
	return textureLod(samplerCubeArray(addr.container), vec4(uv.r, uv.b, -uv.g, addr.page), roughnessValue);
}


float SampleMaterialHeight(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].height.texture;

	float modifier = GetTextureModifier(addr);

	return 0.0 + modifier * (1.0 - SampleTexture(addr, vec3(uv.rg, addr.page), 0).r);//texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page)).r);
}

vec3 SampleAnisotropy(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].anisotropy.texture;

	float modifier = GetTextureModifier(addr);

	vec3 anisotropyDirection = SampleTexture(addr, vec3(uv.r, uv.g, addr.page), 0).rrr; //@TODO(Serge): fix this to be .rgb - won't work at the moment with our content

	anisotropyDirection = anisotropyDirection;

	return (1.0 - modifier) * fs_in.tangent + modifier * fs_in.TBN * anisotropyDirection;// + modifier * (fs_in.TBN * anisotropyDirection);
}

vec3 ParallaxMapping(uint drawID, vec3 uv, vec3 viewDir)
{
	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	Tex2DAddress addr = materials[texIndex].height.texture;

	float modifier = GetTextureModifier(addr);

	const float heightScale = materials[texIndex].heightScale;

	if(modifier <= 0.0 || heightScale <= 0.0)
		return uv;

	float currentLayerDepth = 0.0;

	const float minLayers = 8;
	const float maxLayers = 32;

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
	//z is up so we rotate this... not sure if this is right?
	return texture(samplerCubeArray(addr.container), vec4(uv.r, uv.b, -uv.g, addr.page));
}

float D_GGX(float NoH, float roughness)
{
	float oneMinusNoHSquared = 1.0 - NoH * NoH;

	float a = NoH * roughness;
	float k = roughness / (oneMinusNoHSquared + a * a);
	float d = k * k * (1.0 / PI);
	return clamp(d, 0.0, 1.0);
}

float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH)
{
	// Burley 2012, "Physically-Based Shading at Disney"

	highp float a2 = at * ab;
	highp vec3 d = vec3(ab * ToH, at * BoH, a2 * NoH);
	highp float d2 = dot(d, d);
	highp float b2 = a2 / d2;
	return a2 * b2 * b2 * (1.0 / PI);
}

vec3 F_Schlick(float LoH, vec3 F0)
{
	return F0 + (vec3(1.0) - F0) * pow(1.0 - LoH, 5.0);
}

float F_Schlick(float f0, float f90, float VoH)
{
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

vec3 F_Schlick(const vec3 F0, float F90, float VoH)
{
	return F0 + (F90 - F0) * pow(1.0 - VoH, 5.0);
}

vec3 Fresnel(const vec3 f0, float LoH) 
{
    float f90 = clamp(dot(f0, vec3(50.0 * 0.33)), 0.0, 1.0);
    return F_Schlick(f0, f90, LoH);
}

vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}


float V_SmithGGXCorrelated(float NoV, float NoL, float roughness, float ggxVTerm)
{
	float a2 = roughness * roughness;
	float GGXL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
	float GGXV = NoL * ggxVTerm;
	return clamp(0.5 / (GGXV + GGXL), 0.0, 1.0);
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float lambdaVTerm, float ToL, float BoL, float NoV, float NoL)
{
	float lambdaV = NoL * lambdaVTerm;
	float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
	float v = 0.5 / (lambdaV + lambdaL);
	return clamp(v, 0.0, 1.0);
}

float V_Kelemen(float LoH)
{
	return clamp(0.25 / (LoH * LoH), 0.0, 1.0);
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


float ClearCoatLobe(float clearCoat, float clearCoatRoughness, float NoH, float LoH, out float Fcc)
{
	float clearCoatNoH = NoH;

	float Dcc = D_GGX(clearCoatNoH, clearCoatRoughness);
	float Vcc = V_Kelemen(LoH);
	float F = F_Schlick(0.04, 1.0, LoH) * clearCoat;

	Fcc = F;
	return Dcc * Vcc * F;
}

vec3 BRDF(vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoV, float NoL, float ggxVTerm, vec3 energyCompensation, float roughness, float clearCoat, float clearCoatRoughness, vec3 clearCoatNormal, float shadow)
{
	vec3 H = normalize(V + L);

	
	float NoH = clamp(dot(N, H), 0.0, 1.0);
	float LoH = clamp(dot(L, H), 0.0, 1.0);
	float clearCoatNoH = clamp(dot(clearCoatNormal, H), 0.0, 1.0);


	float D = D_GGX(NoH, roughness);
	vec3 F = F_Schlick(max(dot(H, V), 0.0), F0);
	float VSmith = V_SmithGGXCorrelated(NoV, NoL, roughness, ggxVTerm);

	//specular BRDF
	vec3 Fr =  (D * VSmith) * F;

	//Energy compensation
	//vec3 energyCompensation = 1.0 + F0 * (1.0 / dfg.y - 1.0);

	Fr *= energyCompensation;

	//float denom = 4.0 * NoV * NoL;
	vec3 specular = Fr;// max(denom, 0.001);
	vec3 Fd = diffuseColor * Fd_Lambert(); //Fd_Burley(roughness, NoV, NoL, LoH);

	//vec3 kS = F;
	//vec3 kD = vec3(1.0) - kS;

	float Fcc;
	float clearCoatLobe = ClearCoatLobe(clearCoat, clearCoatRoughness, clearCoatNoH, LoH, Fcc);
	float attenuation = 1.0 - Fcc;

	return ((Fd + specular) * attenuation + clearCoatLobe) * shadow;
}

vec3 BRDF_Anisotropic(float anisotropy, float at, float ab, vec3 anisotropicT, vec3 anisotropicB, vec3 diffuseColor, vec3 N, vec3 V, vec3 L, vec3 F0, float NoV, float ToV, float BoV, float NoL, float lambdaVTerm, float roughness, float shadow)
{
	vec3 H = normalize(V + L);
	vec3 T = anisotropicT ; //TODO(Serge): calculate this by passing in the materials anisotropic direction - probably need to pass in
	vec3 B = anisotropicB; //TODO(Serge): calc based on anisotropic direction

	float LoH = clamp(dot(L, H), 0.0, 1.0);
	float ToL = dot(T, L);
	float BoL = dot(B, L);
	float ToH = dot(T, H);
	float BoH = dot(B, H);
	float NoH = dot(N, H);

	float Da = D_GGX_Anisotropic(at, ab, ToH, BoH, NoH);
	float Va = V_SmithGGXCorrelated_Anisotropic(at, ab, lambdaVTerm, ToL, BoL, NoV, NoL);	
	vec3 F = Fresnel(F0, LoH);

	vec3 Fr = (Da * Va) * F;
	vec3 Fd = diffuseColor * Fd_Lambert();

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;

	return (kD * Fd + Fr) * shadow;
}

vec3 Eval_BRDF(
	float anisotropy,
	float at,
	float ab,
	vec3 anisotropicT,
	vec3 anisotropicB,
	vec3 diffuseColor,
	vec3 N,
	vec3 V,
	vec3 L,
	vec3 F0,
	float NoV,
	float ToV,
	float BoV,
	float NoL,
	float ggxVTerm,
	vec3 energyCompensation,
	float roughness,
	float clearCoat,
	float clearCoatRoughness,
	vec3 clearCoatNormal,
	float shadow)
{
	if(anisotropy != 0.0)
	{
		return BRDF_Anisotropic(anisotropy, at, ab, anisotropicT, anisotropicB, diffuseColor, N, V, L, F0, NoV, ToV, BoV, NoL, ggxVTerm,  roughness, shadow); 
	}
	
	return BRDF(diffuseColor, N, V, L, F0, NoV, NoL, ggxVTerm, energyCompensation, roughness, clearCoat, clearCoatRoughness, clearCoatNormal, shadow);
}

float GetDistanceAttenuation(vec3 posToLight, float falloff)
{
	float distanceSquare = dot(posToLight, posToLight);

    float factor = distanceSquare * falloff;

    float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);

    float attenuation = smoothFactor * smoothFactor;

    return attenuation * 1.0 / max(distanceSquare, 1e-4);
}

vec3 CalculateClearCoatBaseF0(vec3 F0, float clearCoat)
{
	vec3 newF0 = clamp(F0 * (F0 * (0.941892 - 0.263008 * F0) + 0.346479) - 0.0285998, 0.0, 1.0);
	return mix(F0, newF0, clearCoat);
}

float CalculateClearCoatRoughness(float clearCoatPerceptualRoughness)
{
	return clearCoatPerceptualRoughness * clearCoatPerceptualRoughness;
}

vec3 CalcEnergyCompensation(vec3 F0, vec2 dfg)
{
	if(dfg.y == 0)
	{
		return vec3(1);
	}
	return 1.0 + F0 * (1.0 / dfg.y - 1.0);
}

vec3 SpecularDFG(vec3 F0, vec2 dfg)
{
	return mix(dfg.xxx, dfg.yyy, F0);
}

float SpecularAO_Lagarde(float NoV, float visibility, float roughness) {
    // Lagarde and de Rousiers 2014, "Moving Frostbite to PBR"
    return clamp(pow(NoV + visibility, exp2(-16.0 * roughness - 1.0)) - 1.0 + visibility, 0.0, 1.0);
}

void EvalClearCoatIBL(vec3 clearCoatReflectionVector, float clearCoatNoV, float clearCoat, float clearCoatRoughness, float clearCoatPerceptualRoughness, float diffuseAO, inout vec3 Fd, inout vec3 Fr)
{
	float Fc = F_Schlick(0.04, 1.0, clearCoatNoV) * clearCoat;
	float attenuation = 1.0 - Fc;
	Fd *= attenuation;
	Fr *= attenuation;

	float specularAO = SpecularAO_Lagarde(clearCoatNoV, diffuseAO, clearCoatRoughness);
	Fr += SampleMaterialPrefilteredRoughness(clearCoatReflectionVector, clearCoatPerceptualRoughness * numPrefilteredRoughnessMips).rgb * (specularAO * Fc);
}

vec3 GetReflectionVector(float anisotropy, const vec3 anisotropyT, const vec3 anisotropyB, float perceptualRoughness, const vec3 V, const vec3 N)
{
	if(anisotropy != 0.0)
	{
		vec3 anisotropyDirection = anisotropy >= 0.0 ? anisotropyB : anisotropyT;
		vec3 anisotropicTangent = cross(anisotropyDirection, V);
		vec3 anisotripicNormal = cross(anisotropicTangent, anisotropyDirection);
		float bendFactor = abs(anisotropy) * clamp(5.0 * perceptualRoughness, 0.0, 1.0);
		vec3 bentNormal = normalize(mix(N, anisotripicNormal, bendFactor));

		return reflect(-V, bentNormal);
	}

	return reflect(-V, N);
}


vec3 CalculateLightingBRDF(vec3 N, vec3 V, vec3 baseColor, uint drawID, vec3 uv)
{
	vec3 color = vec3(0.0);

	highp uint texIndex = uint(round(uv.z)) + materialOffsets[drawID];
	
	float reflectance = materials[texIndex].reflectance;

	float anisotropy =  materials[texIndex].anisotropy.color.r;

	float metallic = SampleMaterialMetallic(drawID, uv).r ;

	float ao =  SampleMaterialAO(drawID, uv).r;

	float perceptualRoughness = clamp(SampleMaterialRoughness(drawID, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	float roughness = perceptualRoughness * perceptualRoughness;

	vec3 F0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
	
	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	vec3 anisotropicT = normalize(SampleAnisotropy(drawID, uv));
	vec3 anisotropicB = normalize(cross(fs_in.normal, anisotropicT));

	vec3 R = GetReflectionVector(anisotropy, anisotropicT, anisotropicB, perceptualRoughness, V, N);

	float clearCoat = materials[texIndex].clearCoat.color.r;
	vec3 clearCoatNormal = fs_in.normal; //geometric normal
	float clearCoatPerceptualRoughness = clamp(materials[texIndex].clearCoatRoughness.color.r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
	float clearCoatRoughness = CalculateClearCoatRoughness(clearCoatPerceptualRoughness);

	F0 = CalculateClearCoatBaseF0(F0, clearCoat);
	

	float NoV = max(dot(N,V), 0.0);
	float ToV = dot(anisotropicT, V);
	float BoV = dot(anisotropicB, V);
	float clearCoatNoV = max(dot(clearCoatNormal, V), 0.0);

	//this is evaluating the IBL stuff
	vec3 diffuseIrradiance = SampleSkylightDiffuseIrradiance(N).rgb;
	vec3 prefilteredRadiance = SampleMaterialPrefilteredRoughness(R, roughness * numPrefilteredRoughnessMips).rgb;
	vec2 dfg = SampleLUTDFG(vec2(NoV, roughness)).rg;

	vec3 energyCompensation = CalcEnergyCompensation(F0, dfg);
	float specularAO =  SpecularAO_Lagarde(NoV, ao, roughness);

	vec3 E = F_SchlickRoughness(NoV, F0, roughness);
	vec3 Fr = vec3(0);
	Fr += E * prefilteredRadiance;

	Fr *= (specularAO * energyCompensation);

	vec3 Fd = vec3(0);

	Fd += diffuseColor * diffuseIrradiance * (1.0 - E) * ao;

	if(anisotropy == 0.0)
	{
		vec3 clearCoatR = reflect(-V, clearCoatNormal);
		EvalClearCoatIBL(clearCoatR, clearCoatNoV, clearCoat, clearCoatRoughness, clearCoatPerceptualRoughness, ao, Fd, Fr);
	}
	

	color += (Fd + Fr);


	//evaluate the lights
	
	//for standard model
	float a2 = roughness * roughness;
	float ggxVTerm = sqrt((NoV - a2 * NoV) * NoV + a2);

	//for anisotropic model
	float at = max(roughness * (1.0 + anisotropy), MIN_PERCEPTUAL_ROUGHNESS);
	float ab = max(roughness * (1.0 - anisotropy), MIN_PERCEPTUAL_ROUGHNESS);

	if(abs(anisotropy) < 1e-5)
	{
		float ToV = dot(fs_in.tangent, V);
		float BoV = dot(fs_in.bitangent, V);

		ggxVTerm = length(vec3(at * ToV, ab * BoV, NoV));
	}

	vec3 L0 = vec3(0,0,0);

	for(int i = 0; i < numDirectionLights; i++)
	{
	 	DirLight dirLight = dirLights[i];

	 	vec3 L = normalize(-dirLight.direction.xyz);

	 	float NoL = clamp(dot(N, L), 0.0, 1.0);
	 	float clearCoatNoL = clamp(dot(clearCoatNormal, L), 0.0, 1.0);

	 	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity * exposureNearFar.x;

	 	float shadow = 0;

	 	if(dirLight.lightProperties.castsShadowsUseSoftShadows.x > 0)
	 	{
	 	 	shadow = ShadowCalculation(fs_in.fragPos, dirLight.direction.xyz, dirLight.lightProperties.lightID, dirLight.lightProperties.castsShadowsUseSoftShadows.y > 0);
	 	}

	 	vec3 result = Eval_BRDF(anisotropy, at, ab, anisotropicT, anisotropicB, diffuseColor, N, V, L, F0, NoV, ToV, BoV, NoL, ggxVTerm, energyCompensation, roughness, clearCoat, clearCoatRoughness, clearCoatNormal, 1.0 - shadow);

	 	L0 += result * radiance * NoL;
	 }

	for(int i = 0; i < numPointLights; ++i)
	{
		PointLight pointLight = pointLights[i];

		vec3 posToLight = pointLight.position.xyz - fs_in.fragPos;

		vec3 L = normalize(posToLight);

		float NoL = clamp(dot(N, L), 0.0, 1.0);

		float attenuation = GetDistanceAttenuation(posToLight, pointLight.lightProperties.fallOffRadius);

		vec3 radiance = pointLight.lightProperties.color.rgb * attenuation * pointLight.lightProperties.intensity * exposureNearFar.x;

		float shadow = 0;

		if(pointLight.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			vec3 fragToPointLight = fs_in.fragPos - pointLight.position.xyz;

			//float PointLightShadowCalculation(vec3 fragToLight, vec3 viewPos, float farPlane, int64_t lightID, bool softShadows)
			shadow = PointLightShadowCalculation(fragToPointLight, cameraPosTimeW.xyz, pointLight.lightProperties.intensity, pointLight.lightProperties.lightID, pointLight.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}

		vec3 result = Eval_BRDF(anisotropy, at, ab, anisotropicT, anisotropicB, diffuseColor, N, V, L, F0, NoV, ToV, BoV, NoL, ggxVTerm, energyCompensation, roughness, clearCoat, clearCoatRoughness, clearCoatNormal, 1.0 - shadow);
		
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


		vec3 radiance = spotLight.lightProperties.color.rgb * attenuation * spotAngleAttenuation * spotLight.lightProperties.intensity * exposureNearFar.x;


		float shadow = 0;

		if(spotLight.lightProperties.castsShadowsUseSoftShadows.x > 0)
		{
			shadow = SpotLightShadowCalculation(fs_in.fragPos, spotLight.direction.xyz, spotLight.lightProperties.lightID, i, spotLight.lightProperties.castsShadowsUseSoftShadows.y > 0);
		}


		vec3 result = Eval_BRDF(anisotropy, at, ab, anisotropicT, anisotropicB, diffuseColor, N, V, L, F0, NoV, ToV, BoV, NoL, ggxVTerm, energyCompensation, roughness, clearCoat, clearCoatRoughness, clearCoatNormal, 1.0 - shadow);

		L0 += result * radiance * NoL;
	}

	//vec3 kS = F_SchlickRoughness(max(dot(N,V), 0.0), F0, roughness);
	//vec3 kD = 1.0 - kS;
	//kD *= 1.0 - metallic;
 


	//vec3 specular = prefilteredColor * (dfg.y + dfg.x ) * kS;

	//vec3 ambient = (kD * baseColor * diffuseIrradiance + specular) * ao;

	color += L0; //+ambient

	return color;
}

vec3 GetShadowPosOffset(float NoL, vec3 normal, uint cascadeIndex)
{
	const float OFFSET_SCALE = 0.0f;

	float texelSize = 2.0 / shadowMapSizes[cascadeIndex];
	float nmlOffsetScale = clamp(1.0 - NoL, 0.0, 1.0);
	return texelSize * OFFSET_SCALE * nmlOffsetScale * normal;
}

float SampleDirectionShadowMap(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, uint cascadeIndex, int64_t lightID, float depth)
{
	vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
	vec3 coord = vec3(uv, cascadeIndex + gDirectionLightShadowMapPages[int(lightID)]);
	float shadowSample = texture(sampler2DArray(shadowsSurface.container), coord).r;

	return depth > shadowSample ? 1.0 : 0.0;
}

float SampleSpotlightShadowMap(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, int64_t spotlightID, float depth)
{
	vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
	vec3 coord = vec3(uv, gSpotLightShadowMapPages[int(spotlightID)]);
	float shadowSample = texture(sampler2DArray(shadowsSurface.container), coord).r;

	return depth > shadowSample ? 1.0 : 0.0;
}

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);   



float SamplePointlightShadowMap(vec3 fragToLight, vec3 offset, int64_t lightID)
{
	vec4 coord = vec4(fragToLight + offset, gPointLightShadowMapPages[int(lightID)]);
	float shadowSample = texture(samplerCubeArray(pointLightShadowsSurface.container), coord).r;
	return shadowSample;
}

float OptimizedPCF(vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float lightDepth)
{
	//Using FilterSize_ == 3 from https://github.com/TheRealMJP/Shadows/blob/master/Shadows/Mesh.hlsl - SampleShadowMapOptimizedPCF - from the witness
	vec2 uv = shadowPosition.xy * shadowMapSizes[cascadeIndex];

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSizes[cascadeIndex], 1.0 / shadowMapSizes[cascadeIndex] );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0;

	float uw0 = (3 - 2 * s);
	float uw1 = (1 + 2 * s);

	float u0 = (2 - s) / uw0 - 1;
	float u1 = s / uw1 + 1;

	float vw0 = (3 - 2 * t);
	float vw1 = (1 + 2 * t);

	float v0 = (2 - t) / vw0 - 1;
	float v1 = t / vw1 + 1;

	sum += uw0 * vw0 * SampleDirectionShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIndex, lightID, lightDepth);
	sum += uw1 * vw0 * SampleDirectionShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIndex, lightID, lightDepth);
	sum += uw0 * vw1 * SampleDirectionShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightID, lightDepth);
	sum += uw1 * vw1 * SampleDirectionShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIndex, lightID, lightDepth);

	return sum * 0.0625; //1 / 16
}



float SampleShadowCascade(vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float NoL, bool softShadows)
{
	shadowPosition += gBias[cascadeIndex][int(lightID)].xyz;
	shadowPosition *= gScale[cascadeIndex][int(lightID)].xyz;

	if(shadowPosition.z > 1.0)
	{
		return 0.0;
	}

	float lightDepth = shadowPosition.z;
	float bias = max(0.001 * (1.0 - NoL), 0.0005);
	if(cascadeIndex <= NUM_FRUSTUM_SPLITS - 1)
	{
		bias *= 1.0 / (gPartitions.intervalEnd[cascadeIndex] - gPartitions.intervalBegin[cascadeIndex]);
	}

	lightDepth -= bias;


	if(softShadows)
	{
		return SoftShadow(shadowPosition, cascadeIndex, lightID, lightDepth);
	}
	else
	{
		return OptimizedPCF(shadowPosition, cascadeIndex, lightID, lightDepth);
	}
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 lightDir, int64_t lightID, bool softShadows)
{
	vec3 normal = normalize(fs_in.normal);
	vec3 viewDir = normalize(cameraPosTimeW.xyz - fs_in.fragPos);

	int lightIndex = int(lightID);

	//don't put the shadow on the back of a surface
	if(dot(viewDir, normal) < 0.0)
	{
		return 0.0; //or maybe return 1?
	}

	float NoL = max(dot(-lightDir, normal), 0.0);

	vec4 projectionPosInCSMSplitSpace = (gShadowMatrix[lightIndex] * vec4(fragPosWorldSpace, 1.0));

	vec3 projectionPos = projectionPosInCSMSplitSpace.xyz;

	uint layer = NUM_FRUSTUM_SPLITS - 1;



	for(int i = int(layer); i >= 0; --i)
	{
		vec3 scale = gScale[i][lightIndex].xyz;
		vec3 bias = gBias[i][lightIndex].xyz;

		vec3 cascadePos = projectionPos + bias;
		cascadePos *= scale;
		cascadePos = abs(cascadePos - 0.5f);
		if(cascadePos.x <= 0.5 && cascadePos.y <= 0.5 && cascadePos.z <= 0.5)
		{
			layer = i;
		}
	}


	vec3 offset = GetShadowPosOffset(NoL, normal, layer);

	vec3 samplePos = fragPosWorldSpace + offset;

	vec3 shadowPosition = (gShadowMatrix[lightIndex] * vec4(samplePos, 1.0)).xyz;

	float shadowVisibility = SampleShadowCascade(shadowPosition, layer, lightID, NoL, softShadows);

	
	const float BLEND_THRESHOLD = 0.1f;

    float nextSplit = gPartitions.intervalEnd[layer];//gPartitions[layer].intervalEndBias.x;
    float splitSize = layer == 0 ? nextSplit : nextSplit - gPartitions.intervalEnd[layer-1];//gPartitions[layer - 1].intervalEndBias.x;
    float fadeFactor = (nextSplit - gl_FragDepth) / splitSize;

    vec3 cascadePos = projectionPos + gBias[layer][lightIndex].xyz;//gPartitions[layer].intervalEndBias.yzw;
    cascadePos *= gScale[layer][lightIndex].xyz;//gPartitions[layer].intervalBeginScale.yzw;
    cascadePos = abs(cascadePos * 2.0 - 1.0);
    float distToEdge = 1.0 - max(max(cascadePos.x, cascadePos.y), cascadePos.z);
    fadeFactor = max(distToEdge, fadeFactor);

    if(fadeFactor <= BLEND_THRESHOLD && layer != NUM_FRUSTUM_SPLITS - 1)
    {
    	vec3 nextCascadeOffset = GetShadowPosOffset(NoL, normal, layer) / abs(gScale[layer+1][lightIndex].z);
    	vec3 nextCascadeShadowPosition = (gShadowMatrix[lightIndex] * vec4(fragPosWorldSpace + nextCascadeOffset, 1.0)).xyz;

    	float nextSplitVisibility = SampleShadowCascade(nextCascadeShadowPosition, layer + 1, lightID, NoL, softShadows);

    	float lerpAmt = smoothstep(0.0, BLEND_THRESHOLD, fadeFactor);
    	shadowVisibility = mix(nextSplitVisibility, shadowVisibility, lerpAmt);
    }

	return shadowVisibility;
}

float SpotLightOptimizedPCF(vec3 shadowPosition, int64_t lightID, float lightDepth)
{
	//Using FilterSize_ == 3 from https://github.com/TheRealMJP/Shadows/blob/master/Shadows/Mesh.hlsl - SampleShadowMapOptimizedPCF - from the witness
	vec2 uv = shadowPosition.xy * shadowMapSizes[0];

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSizes[0], 1.0 / shadowMapSizes[0] );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	float s = (uv.x + 0.5 - base_uv.x);
	float t = (uv.y + 0.5 - base_uv.y);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float sum = 0;

	float uw0 = (3 - 2 * s);
	float uw1 = (1 + 2 * s);

	float u0 = (2 - s) / uw0 - 1;
	float u1 = s / uw1 + 1;

	float vw0 = (3 - 2 * t);
	float vw1 = (1 + 2 * t);

	float v0 = (2 - t) / vw0 - 1;
	float v1 = t / vw1 + 1;

	sum += uw0 * vw0 * SampleSpotlightShadowMap(base_uv, u0, v0, shadowMapSizeInv, lightID, lightDepth);
	sum += uw1 * vw0 * SampleSpotlightShadowMap(base_uv, u1, v0, shadowMapSizeInv, lightID, lightDepth);
	sum += uw0 * vw1 * SampleSpotlightShadowMap(base_uv, u0, v1, shadowMapSizeInv, lightID, lightDepth);
	sum += uw1 * vw1 * SampleSpotlightShadowMap(base_uv, u0, v1, shadowMapSizeInv, lightID, lightDepth);

	return sum * 0.0625; //1 / 16
}

float SpotLightPenumbra(float gradientNoise, vec2 shadowMapUV, float depth, int samplesCount, vec2 shadowMapSizeInv, int64_t lightID)
{
	float avgBlockerDepth = 0.0;
	float blockersCount = 0.0;


	for(int i = 0; i < samplesCount; ++i)
	{
		vec2 sampleUV = VogelDiskSample(i, samplesCount, gradientNoise);
		sampleUV = shadowMapUV + PENUMBRA_FILTER_SCALE * sampleUV;

		float sampleDepth = SampleSpotlightShadowMap(sampleUV, 0, 0, shadowMapSizeInv, lightID, depth);

		if(sampleDepth < depth)
		{
			avgBlockerDepth += sampleDepth;
			blockersCount += 1.0;
		}
	}

	if(blockersCount > 0.0)
	{
		avgBlockerDepth /= blockersCount;
		return AvgBlockersDepthToPenumbra(depth, avgBlockerDepth);
	}

	return 0.0;
}

float SpotLightSoftShadow(vec3 shadowPosition, int64_t lightID, float lightDepth)
{
	vec2 uv = shadowPosition.xy * shadowMapSizes[0];

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSizes[0], 1.0 / shadowMapSizes[0] );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;


	float gradientNoise = TWO_PI * InterleavedGradientNoise(gl_FragCoord.xy);

	//float Penumbra(float gradientNoise, vec2 shadowMapUV, float depth, int samplesCount, vec2 shadowMapSizeInv, uint cascadeIndex)
	float penumbra = SpotLightPenumbra(gradientNoise, base_uv, lightDepth, int(NUM_SOFT_SHADOW_SAMPLES), shadowMapSizeInv, lightID);

	float shadow = 0.0;
	for(int i = 0; i < NUM_SOFT_SHADOW_SAMPLES; ++i)
	{
		vec2 sampleUV = VogelDiskSample(i, int(NUM_SOFT_SHADOW_SAMPLES), gradientNoise);
		sampleUV = base_uv + sampleUV * penumbra * SHADOW_FILTER_MAX_SIZE;

		shadow += SampleSpotlightShadowMap(sampleUV, 0, 0, shadowMapSizeInv, lightID, lightDepth);
	}

	return shadow / NUM_SOFT_SHADOW_SAMPLES;
}


float SpotLightShadowCalculation(vec3 fragPosWorldSpace, vec3 lightDir, int64_t lightID, int index, bool softShadows)
{

	vec3 normal = normalize(fs_in.normal);
	vec3 viewDir = normalize(cameraPosTimeW.xyz - fragPosWorldSpace);

	//don't put the shadow on the back of a surface
	if(dot(viewDir, normal) < 0.0)
	{
		return 1.0; //or maybe return 1?
	}

	float NoL = max(dot(-lightDir, normal), 0.0);

	vec4 posLightSpace = spotLights[index].lightSpaceMatrix * vec4(fragPosWorldSpace, 1);

	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;

	projCoords = projCoords * 0.5 + 0.5;

	float bias = max(0.005 * (1.0 - NoL), 0.0005);

	float lightDepth = projCoords.z - bias;

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSizes[0], 1.0 / shadowMapSizes[0] );

	if(softShadows)
	{
		return SpotLightSoftShadow(projCoords, lightID, lightDepth);
	}

	return SpotLightOptimizedPCF(projCoords, lightID, lightDepth);
}


float PointLightShadowCalculation(vec3 fragToLight, vec3 viewPos, float farPlane, int64_t lightID, bool softShadows)
{

	vec3 V = viewPos - fs_in.fragPos;
	float viewDistance = length(V);
	if(dot(V, fragToLight) > 0)
	{
		return 0;
	}

	float bias = 0.05;
	float lightDepth = length(fragToLight) - bias;


	if(!softShadows)
	{
		float closestDepth = SamplePointlightShadowMap(fragToLight, vec3(0), lightID);
		closestDepth *= farPlane;

		return lightDepth > closestDepth ? 1.0 : 0.0;
	}

	
	int samples = 20;
	float shadow = 0.0;
	
	float diskRadius = (1.0 + (viewDistance / farPlane)) / 150.0;

	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = SamplePointlightShadowMap(fragToLight, sampleOffsetDirections[i]*diskRadius, lightID);
		closestDepth *= farPlane;
		if(lightDepth > closestDepth)
		{
			shadow += 1.0;
		}

	}

	return shadow / (float)samples;
}

float SoftShadow(vec3 shadowPosition, uint cascadeIndex, int64_t lightID, float lightDepth)
{
	vec2 uv = shadowPosition.xy * shadowMapSizes[cascadeIndex];

	vec2 shadowMapSizeInv = vec2( 1.0 / shadowMapSizes[cascadeIndex], 1.0 / shadowMapSizes[cascadeIndex] );

	vec2 base_uv;
	base_uv.x = floor(uv.x + 0.5);
	base_uv.y = floor(uv.y + 0.5);

	base_uv -= vec2(0.5, 0.5);
	base_uv *= shadowMapSizeInv;

	float gradientNoise = TWO_PI * InterleavedGradientNoise(gl_FragCoord.xy);

	float penumbra = Penumbra(gradientNoise, base_uv, lightDepth, int(NUM_SOFT_SHADOW_SAMPLES), shadowMapSizeInv, cascadeIndex, lightID);

	float shadow = 0.0;
	for(int i = 0; i < NUM_SOFT_SHADOW_SAMPLES; ++i)
	{
		vec2 sampleUV = VogelDiskSample(i, int(NUM_SOFT_SHADOW_SAMPLES), gradientNoise);
		sampleUV = base_uv + sampleUV * penumbra * SHADOW_FILTER_MAX_SIZE;

		shadow += SampleDirectionShadowMap(sampleUV, 0, 0, shadowMapSizeInv, cascadeIndex, lightID, lightDepth);
	}

	return shadow / NUM_SOFT_SHADOW_SAMPLES;
}

vec2 VogelDiskSample(int sampleIndex, int samplesCount, float phi)
{
	float GOLDEN_ANGLE = 2.4;

	float r = sqrt(sampleIndex + 0.5) / sqrt(samplesCount);
	float theta = sampleIndex * GOLDEN_ANGLE + phi;
	
	return vec2(r * cos(theta), r * sin(theta));
}

float InterleavedGradientNoise(vec2 screenPosition)
{
	vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
  	return fract(magic.z * fract(dot(screenPosition, magic.xy)));
}

//float SampleShadowMap(vec2 base_uv, float u, float v, vec2 shadowMapSizeInv, uint cascadeIndex, float depth)
float Penumbra(float gradientNoise, vec2 shadowMapUV, float depth, int samplesCount, vec2 shadowMapSizeInv, uint cascadeIndex, int64_t lightID)
{
	float avgBlockerDepth = 0.0;
	float blockersCount = 0.0;

	for(int i = 0; i < samplesCount; ++i)
	{
		vec2 sampleUV = VogelDiskSample(i, samplesCount, gradientNoise);
		sampleUV = shadowMapUV + PENUMBRA_FILTER_SCALE * sampleUV;

		float sampleDepth = SampleDirectionShadowMap(sampleUV, 0, 0, shadowMapSizeInv, cascadeIndex, lightID, depth);

		if(sampleDepth < depth)
		{
			avgBlockerDepth += sampleDepth;
			blockersCount += 1.0;
		}
	}

	if(blockersCount > 0.0)
	{
		avgBlockerDepth /= blockersCount;
		//return AvgBlockersDepthToPenumbra(depth, avgBlockerDepth);
	}

	return AvgBlockersDepthToPenumbra(depth, avgBlockerDepth);
	//return 0;
}

float AvgBlockersDepthToPenumbra(float depth, float avgBlockersDepth)
{
	float penumbra = (depth - avgBlockersDepth) / avgBlockersDepth;
	penumbra *= penumbra;
	return clamp(penumbra , 0.0, 1.0);
}