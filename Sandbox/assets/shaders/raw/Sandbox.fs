#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;
const uint MAX_NUM_LIGHTS = 50;

layout (location = 0) out vec4 FragColor;

#define PI 3.141596
#define MIN_PERCEPTUAL_ROUGHNESS 0.045
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
	Tex2DAddress anisotropyTexture1;

	vec3 baseColor;
	float specular;
	float roughness;
	float metallic;
	float reflectance;
	float ambientOcclusion;
	float clearCoat;
	float clearCoatRoughness;
	float anisotropy;
	float heightScale;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposure;
    vec4 cascadePlanes;
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

layout (std140, binding = 0) uniform Matrices 
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
};

//@NOTE(Serge): this is in the order of the render target surfaces in RenderTarget.h
layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
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

	//section for shadows in the fragment shader - speeds up the shadow calculations
	vec3 fragPosViewSpace;
	vec4 fragPosLightSpace[NUM_FRUSTUM_SPLITS]; //@TODO(Serge): this is only one light...

} fs_in;

vec4 splitColors[NUM_FRUSTUM_SPLITS] = {vec4(2, 0.0, 0.0, 1.0), vec4(0.0, 2, 0.0, 1.0), vec4(0.0, 0.0, 2, 1.0), vec4(2, 2, 0.0, 1.0)};

vec3 CalculateClearCoatBaseF0(vec3 F0, float clearCoat);

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

float ShadowCalculation(vec3 fragePosWorldSpace, vec3 lightDir, mat4 lightViews[NUM_FRUSTUM_SPLITS], mat4 lightProjs[NUM_FRUSTUM_SPLITS]);

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
	vec4 fragPosViewSpace = view * vec4(fs_in.fragPos, 1.0);

	float depthValue = abs(fragPosViewSpace.z);

	int layer = -1;
	for(int i = 0; i < NUM_FRUSTUM_SPLITS; ++i)
	{
		if(depthValue < cascadePlanes[i])
		{
			layer = i;
			break;
		}
	}

	if(layer == -1)
	{
		layer = NUM_FRUSTUM_SPLITS;
	}

	return splitColors[layer];
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

	if(sampledColor.a < 0.0001)
		discard;

	vec3 norm = SampleMaterialNormal(fs_in.drawID, texCoords).rgb;

	vec3 lightingResult = CalculateLightingBRDF(norm, viewDir, sampledColor.rgb, fs_in.drawID, texCoords);


	vec3 emission = SampleMaterialEmission(fs_in.drawID, texCoords).rgb;




	


	//FragColor = vec4(texCoords.x, texCoords.y, 0, 1.0);
	FragColor = vec4(lightingResult + emission , 1.0) ;//* DebugFrustumSplitColor();

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

	normalMapNormal = normalize(normalMapNormal * 2.0 - 1.0);

	normalMapNormal = normalize(fs_in.TBN * normalMapNormal);

	return  (1.0 - modifier) * vec4(fs_in.normal, 1) +  modifier * vec4(normalMapNormal, 1);
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
	//@TODO(Serge): put this back to roughnessTexture1
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].roughnessTexture1;

	float modifier = GetTextureModifier(addr);

	//@TODO(Serge): put this back to not using the alpha
	return(1.0 - modifier) * vec4(materials[texIndex].roughness) + modifier * (vec4(texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page)).r) );
}

vec4 SampleMaterialAO(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].aoTexture1;

	float modifier = GetTextureModifier(addr);

	return (1.0 - modifier) * vec4(1.0) + modifier * texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page));
}

vec4 SampleMaterialPrefilteredRoughness(vec3 uv, float roughnessValue)
{
	Tex2DAddress addr = skylight.prefilteredRoughnessTexture;
	//z is up so we rotate this... not sure if this is right?
	return textureLod(samplerCubeArray(addr.container), vec4(uv.r, uv.b, -uv.g, addr.page), roughnessValue);
}


float SampleMaterialHeight(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].heightTexture1;

	float modifier = GetTextureModifier(addr);

	return 0.0 + modifier * (1.0 - texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page)).r);
}

vec3 SampleAnisotropy(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].anisotropyTexture1;

	float modifier = GetTextureModifier(addr);

	vec3 anisotropyDirection = texture(sampler2DArray(addr.container), vec3(uv.rg, addr.page)).rrr; //@TODO(Serge): fix this to be .rgb - won't work at the moment with our content

	anisotropyDirection = anisotropyDirection;

	return (1.0 - modifier) * fs_in.tangent + modifier * fs_in.TBN * anisotropyDirection;// + modifier * (fs_in.TBN * anisotropyDirection);
}

vec3 ParallaxMapping(uint drawID, vec3 uv, vec3 viewDir)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].heightTexture1;

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

	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	
	float reflectance = materials[texIndex].reflectance;

	float anisotropy =  materials[texIndex].anisotropy;

	float metallic = SampleMaterialMetallic(drawID, uv).r;

	float ao =  SampleMaterialAO(drawID, uv).r;

	float perceptualRoughness = clamp(SampleMaterialRoughness(drawID, uv).r, MIN_PERCEPTUAL_ROUGHNESS, 1.0);

	float roughness = perceptualRoughness * perceptualRoughness;

	vec3 F0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
	
	vec3 diffuseColor = (1.0 - metallic) * baseColor;

	vec3 anisotropicT = normalize(SampleAnisotropy(drawID, uv));
	vec3 anisotropicB = normalize(cross(fs_in.normal, anisotropicT));

	vec3 R = GetReflectionVector(anisotropy, anisotropicT, anisotropicB, perceptualRoughness, V, N);

	float clearCoat = materials[texIndex].clearCoat;
	vec3 clearCoatNormal = fs_in.normal; //geometric normal
	float clearCoatPerceptualRoughness = clamp(materials[texIndex].clearCoatRoughness, MIN_PERCEPTUAL_ROUGHNESS, 1.0);
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

	 	vec3 radiance = dirLight.lightProperties.color.rgb * dirLight.lightProperties.intensity;

	 	float shadow = ShadowCalculation(fs_in.fragPos, dirLight.direction.xyz, dirLight.lightSpaceMatrixData.lightViewMatrices, dirLight.lightSpaceMatrixData.lightProjMatrices);

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

		vec3 radiance = pointLight.lightProperties.color.rgb * attenuation * pointLight.lightProperties.intensity * exposure.x;

		vec3 result = Eval_BRDF(anisotropy, at, ab, anisotropicT, anisotropicB, diffuseColor, N, V, L, F0, NoV, ToV, BoV, NoL, ggxVTerm, energyCompensation, roughness, clearCoat, clearCoatRoughness, clearCoatNormal, 1.0);
		
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

		vec3 result = Eval_BRDF(anisotropy, at, ab, anisotropicT, anisotropicB, diffuseColor, N, V, L, F0, NoV, ToV, BoV, NoL, ggxVTerm, energyCompensation, roughness, clearCoat, clearCoatRoughness, clearCoatNormal, 1.0);

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

float SampleShadowMap(vec2 uv, float page)
{
	vec3 coord = vec3(uv, page);
	return texture(sampler2DArray(shadowsSurface.container), coord).r;
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 lightDir, mat4 lightViews[NUM_FRUSTUM_SPLITS], mat4 lightProjs[NUM_FRUSTUM_SPLITS])
{


	vec3 normal = normalize(fs_in.normal);
	vec3 viewDir = normalize(cameraPosTimeW.xyz - fs_in.fragPos);

	//don't put the shadow on the back of a surface
	if(dot(viewDir, normal) < 0.0)
	{
		return 1.0; //or maybe return 1?
	}

	float depthValue = abs(fs_in.fragPosViewSpace.z);

	int layer = -1;

	for(int i = 0; i < NUM_FRUSTUM_SPLITS; ++i)
	{
		if(depthValue < cascadePlanes[i])
		{
			layer = i;
			break;
		}
	}

	if(layer == -1)
	{
		return 0.0;
	}

	//layer = 0;

	vec3 projCoords = fs_in.fragPosLightSpace[layer].xyz / fs_in.fragPosLightSpace[layer].w;

	projCoords = projCoords * 0.5 + 0.5;

	float currentDepth = projCoords.z;
	if(currentDepth > 1.0)
	{
		return 0.0;
	}
	
	float bias = max(0.005 * (1.0 - dot(normal, -lightDir)), 0.0005);
	if(layer == NUM_FRUSTUM_SPLITS)
	{
		bias *= 1 / (cascadePlanes[NUM_FRUSTUM_SPLITS - 1] * 0.15);
	}
	else if(layer < NUM_FRUSTUM_SPLITS -1)
	{
		bias *= 1 / (cascadePlanes[layer] * 0.15);
	}

	

	//float shadow = (currentDepth - bias) > shadowMapSample ? 1.0 : 0.0;

	//@TODO(Serge): PCF stuff would be here	
	float shadow = 0.0;//(currentDepth - bias) > SampleShadowMap(projCoords.xy, layer);
    vec2 texelSize = 1.0 / vec2(textureSize(sampler2DArray(shadowsSurface.container), 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = SampleShadowMap(projCoords.xy + vec2(x,y) * texelSize, layer);
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;

	

	return shadow;
}