#version 450 core

#extension GL_NV_gpu_shader5 : enable
//#define NUM_FRUSTUM_SPLITS 4

layout(location = 0) out vec4 oOutputColor;

//https://github.com/man-in-black382/EARenderer/blob/master/EARenderer/Engine/OpenGL/Extensions/Shaders/Postprocessing/Reflections/SSRConeTracing.frag
//http://roar11.com/2015/07/screen-space-glossy-reflections/

#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Input/UniformBuffers/SSRParams.glsl"
// struct Tex2DAddress
// {
// 	uint64_t  container;
// 	float page;
// 	int channel;
// };

// layout (std140, binding = 0) uniform Matrices
// {
// 	mat4 projection;
// 	mat4 view;
// 	mat4 skyboxView;
// 	mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
// 	mat4 inverseProjection;
// 	mat4 inverseView;
// 	mat4 vpMatrix;
// 	mat4 prevProjection;
// 	mat4 prevView;
// 	mat4 prevVPMatrix;
// };

// layout (std140, binding = 1) uniform Vectors
// {
// 	vec4 cameraPosTimeW;
// 	vec4 exposureNearFar;
// 	vec4 cascadePlanes;
// 	vec4 shadowMapSizes;
// 	vec4 fovAspectResXResY;
// 	uint64_t frame;
// 	vec2 clusterScaleBias;
// 	uvec4 tileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
// 	vec4 jitter; // {currJitterX, currJitterY, prevJitterX, prevJitterY}
// };

// layout (std140, binding = 2) uniform Surfaces
// {
// 	Tex2DAddress gBufferSurface;
// 	Tex2DAddress shadowsSurface;
// 	Tex2DAddress compositeSurface;
// 	Tex2DAddress zPrePassSurface;
// 	Tex2DAddress pointLightShadowsSurface;
// 	Tex2DAddress ambientOcclusionSurface;
// 	Tex2DAddress ambientOcclusionDenoiseSurface;
// 	Tex2DAddress zPrePassShadowsSurface[2];
// 	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
// 	Tex2DAddress normalSurface;
// 	Tex2DAddress specularSurface;
// 	Tex2DAddress ssrSurface;
// 	Tex2DAddress convolvedGBUfferSurface[2];
// 	Tex2DAddress ssrConeTracedSurface;
// 	Tex2DAddress bloomDownSampledSurface;
// 	Tex2DAddress bloomBlurSurface;
// 	Tex2DAddress bloomUpSampledSurface;
// };

// layout (std140, binding = 4) uniform SSRParams
// {
// 	float ssr_stride;
// 	float ssr_zThickness;
// 	int ssr_rayMarchIterations;
// 	float ssr_strideZCutoff;
	
// 	Tex2DAddress ssr_ditherTexture;
	
// 	float ssr_ditherTilingFactor;
// 	int ssr_roughnessMips;
// 	int ssr_coneTracingSteps;
// 	float ssr_maxFadeDistance;
	
// 	float ssr_fadeScreenStart;
// 	float ssr_fadeScreenEnd;
// 	float ssr_maxDistance;
// };


vec3 F_Schlick(const vec3 F0, float F90, float VoH)
{
	return F0 + (F90 - F0) * pow(1.0 - VoH, 5.0);
}

const float kMaxSpecularExponent = 16.0;
const float kSpecularBias = 1.0;

float SpecularPowerToConeAngle(float specularPower) {
    // based on phong distribution model
    if(specularPower >= exp2(kMaxSpecularExponent)) {
        return 0.0f;
    }
    const float xi = 0.244f;
    float exponent = 1.0f / (specularPower + 1.0f);
    return acos(pow(xi, exponent));
}

float GlossToSpecularPower(float gloss) 
{
    return exp2(kMaxSpecularExponent * gloss + kSpecularBias);
}

float IsoscelesTriangleOpposite(float adjacentLength, float coneTheta) {
    return 2.0f * tan(coneTheta) * adjacentLength;
}

// a - opposite length of the triangle
// h - adjacent length of the triangle
float IsoscelesTriangleInscribedCircleRadius(float a, float h) {
    float a2 = a * a;
    float fh2 = 4.0f * h * h;
    return (a * (sqrt(a2 + fh2) - a)) / (4.0f * h);
}

vec4 ConeSampleWeightedColor(vec2 samplePos, float mipChannel, float gloss) {
    vec3 sampleColor = textureLod(sampler2DArray(convolvedGBUfferSurface[0].container), vec3(samplePos, convolvedGBUfferSurface[0].page), mipChannel).rgb;
    return vec4(sampleColor * gloss, gloss);
}

float IsoscelesTriangleNextAdjacent(float adjacentLength, float incircleRadius) {
    // subtract the diameter of the incircle to get the adjacent side of the next level on the cone
    return adjacentLength - (incircleRadius * 2.0f);
}

vec3 DecodeNormal(vec2 enc)
{
	vec3 n;

	n.xy = enc * 2 - 1;
	n.z = sqrt(1 - dot(n.xy, n.xy));
	return n;
}

vec3 LoadNormal(vec2 uv)
{
	vec3 coord = vec3(uv.r, uv.g, normalSurface.page );
	vec2 normal = texture(sampler2DArray(normalSurface.container), coord).rg;

	return DecodeNormal(normal);
}

float SampleTextureF(Tex2DAddress tex, vec2 uv, vec2 offset)
{
	vec3 texCoord = vec3(uv + offset, tex.page);
	return texture(sampler2DArray(tex.container), texCoord).r;
}

vec3 GetWorldPosition(vec2 uv)
{
	float depth = SampleTextureF(zPrePassSurface, uv, vec2(0)) * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;

	vec4 worldSpacePosition = inverseView * viewSpacePosition;

	return worldSpacePosition.xyz;
}

vec3 GetViewPosition(vec2 uv)
{
	float depth = SampleTextureF(zPrePassSurface, uv, vec2(0)) * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;

	return viewSpacePosition.xyz;
}

vec3 GetViewPosition(vec2 uv, float depth)
{
	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;

	viewSpacePosition /= viewSpacePosition.w;

	return viewSpacePosition.xyz;
}


in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


vec3 TraceCones(float gloss, vec4 rayHitInfo);

float LuminanceFromRGB(vec3 rgb) {
    vec3 factors = vec3(0.2126, 0.7152, 0.0722);
    return dot(rgb, factors);
}

void main()
{
	vec2 uv = fs_in.texCoords.xy;
	vec4 rayHitInfo = texture(sampler2DArray(ssrSurface.container), vec3(uv, ssrSurface.page));

	if(rayHitInfo.a <= 0.0f)
	{
		oOutputColor = vec4(0);
		return;
	}

	vec3 worldPosition = GetWorldPosition(uv);
	vec3 viewPosition = GetViewPosition(uv);
	vec3 reflectedPointWorldPosition = GetWorldPosition(rayHitInfo.xy);

	vec3 normal = LoadNormal(uv);

	vec3 V = normalize(cameraPosTimeW.xyz - worldPosition);

	vec3 H;

	float attenuation = rayHitInfo.a;
	bool rayHitDetected = attenuation > 0.0;

	if(rayHitDetected)
	{
		vec3 L = normalize(reflectedPointWorldPosition - worldPosition);
		H = normalize(L + V);
	}
	else
	{
		H = normalize(normal + V);
	}

	vec4 specular = texture(sampler2DArray(specularSurface.container), vec3(uv, specularSurface.page));
	vec3 F0 = specular.rgb;

	vec3 Ks = F_Schlick(F0, 1.0f, max(dot(V, H), 0.0));

	vec3 reflectedColor = TraceCones(specular.a, rayHitInfo);

	vec3 raySS = rayHitInfo.xyz;

	vec2 boundary = abs(raySS.xy - vec2(0.5f)) * 2.0f;
	const float fadeDiffRcp = 1.0f / (ssr_fadeScreenEnd - ssr_fadeScreenStart);
	float fadeOnBorder = 1.0f - clamp((boundary.x - ssr_fadeScreenStart) * fadeDiffRcp, 0.0, 1.0);
	fadeOnBorder *= 1.0f - clamp((boundary.y - ssr_fadeScreenStart) * fadeDiffRcp, 0.0, 1.0);
	fadeOnBorder = smoothstep(0.0, 1.0, fadeOnBorder);

	vec3 R = normalize(reflectedPointWorldPosition - worldPosition);

	R = (view * vec4(R, 1.0)).xyz;
	V = (view * vec4(V, 1.0)).xyz;

	float rdotv = dot(V, R);

	vec3 rayHitViewPosition = GetViewPosition(raySS.xy, raySS.z);

	float fadeOnDistance =  1.0f - clamp(distance(rayHitViewPosition, viewPosition) / ssr_maxFadeDistance, 0.0, 1.0);

	float fadeOnPerpendicular = clamp(mix(0.0, 1.0, clamp(rdotv, 0.0, 1.0) * 4.0), 0.0, 1.0);

	float fadeOnRoughness = clamp(mix(0.0, 1.0, specular.a*4.0), 0.0, 1.0);

	float totalFade = fadeOnBorder * fadeOnDistance * fadeOnPerpendicular * fadeOnRoughness;

	reflectedColor = mix(vec3(0), reflectedColor, totalFade);

	reflectedColor *= Ks;

	oOutputColor = vec4(reflectedColor, 1.0);
}

vec3 TraceCones(float gloss, vec4 rayHitInfo)
{
	if(rayHitInfo.a <= 0.0)
	{
		return vec3(0);
	}

	float depth = SampleTextureF(zPrePassSurface, fs_in.texCoords.xy, vec2(0));

	vec3 raySS = rayHitInfo.xyz;
	vec3 positionSS = vec3(fs_in.texCoords.xy, depth);

	float specularPower = GlossToSpecularPower(gloss);

	float coneTheta = SpecularPowerToConeAngle(specularPower) * 0.5;

	vec2 deltaP = raySS.xy - positionSS.xy;
	float adjacentLength = length(deltaP);
	vec2 adjacentUnit = normalize(deltaP);

	vec4 totalReflectedColor = vec4(0.0);
	float remainingAlpha = 1.0f;
	float maxMipLevel = float(ssr_roughnessMips) - 1.0f;

	float glossMult = gloss;

	vec2 texSize = vec2(textureSize(sampler2DArray(convolvedGBUfferSurface[0].container), 0));

	for(int i = 0; i < ssr_coneTracingSteps; ++i)
	{
		float oppositeLength = IsoscelesTriangleOpposite(adjacentLength, coneTheta);

		float incircleSize = IsoscelesTriangleInscribedCircleRadius(oppositeLength, adjacentLength);

		vec2 samplePos = positionSS.xy + adjacentUnit * (adjacentLength - incircleSize);

		float mipChannel = clamp(log2(incircleSize * max(texSize.x, texSize.y)), 0.0, maxMipLevel);

		vec4 newColor = ConeSampleWeightedColor(samplePos, mipChannel, glossMult);

		remainingAlpha -= newColor.a;

		if(remainingAlpha < 0.0f)
		{
			newColor.rgb *= (1.0f - abs(remainingAlpha));
		}

		totalReflectedColor += newColor;

		if(totalReflectedColor.a >= 1.0f)
		{
			break;
		}

		adjacentLength = IsoscelesTriangleNextAdjacent(adjacentLength, incircleSize);
		glossMult *= gloss;
	}

	return totalReflectedColor.rgb * rayHitInfo.a;
}