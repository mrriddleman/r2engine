#version 450 core

#extension GL_NV_gpu_shader5 : enable
#define NUM_FRUSTUM_SPLITS 4
#define GLOBAL_UP vec3(0, 0, 1)
//https://sakibsaikia.github.io/graphics/2016/12/26/Screen-Space-Reflection-in-Killing-Floor-2.html

layout(location = 0) out vec4 oRayHitInfo;



struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

layout (std140, binding = 0) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 skyboxView;
	mat4 cameraFrustumProjections[NUM_FRUSTUM_SPLITS];
	mat4 inverseProjection;
	mat4 inverseView;
	mat4 vpMatrix;
	mat4 prevProjection;
	mat4 prevView;
	mat4 prevVPMatrix;
};

layout (std140, binding = 1) uniform Vectors
{
	vec4 cameraPosTimeW;
	vec4 exposureNearFar;
	vec4 cascadePlanes;
	vec4 shadowMapSizes;
	vec4 fovAspectResXResY;
	uint64_t frame;
	vec2 clusterScaleBias;
	uvec4 tileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
	vec4 jitter; // {currJitterX, currJitterY, prevJitterX, prevJitterY}
};

layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
	Tex2DAddress ambientOcclusionDenoiseSurface;
	Tex2DAddress zPrePassShadowSurface[2]; //current in 0
	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
	Tex2DAddress normalSurface;
	Tex2DAddress specularSurface;
};

layout (std140, binding = 4) uniform SSRParams
{
	float ssr_maxRayMarchStep;
	float ssr_zThickness;
	int ssr_rayMarchIterations;
	int ssr_maxBinarySearchSamples;
	Tex2DAddress ssr_ditherTexture;
	float ssr_ditherTilingFactor;
	int ssr_roughnessMips;
	int ssr_coneTracingSteps;
	// float ssr_strideZCutoff;
	// float ssr_stride;
	// float ssr_maxDistance;

};

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

bool IsReflectedBackToCamera(vec3 reflection, vec3 fragToCamera, inout float attenuationFactor)
{
	// This will check the direction of the reflection vector with the view direction,
    // and if they are pointing in the same direction, it will drown out those reflections
    // since we are limited to pixels visible on screen. Attenuate reflections for angles between
    // 60 degrees and 75 degrees, and drop all contribution beyond the (-60,60)  degree range

	attenuationFactor = 1.0f - smoothstep(0.25f, 0.5f, dot(fragToCamera, reflection));

	return attenuationFactor <= 0;
}

bool IsSamplingOutsideViewport(vec3 raySample, inout float attenuationFactor)
{
	// Any rays that march outside the screen viewport will not have any valid pixel information. These need to be dropped.
	vec2 uvSamplingAttenuation = smoothstep(0.0f, 0.05f, raySample.xy) * (1.0f - smoothstep(0.95f, 1.0f, raySample.xy));
	attenuationFactor = uvSamplingAttenuation.x * uvSamplingAttenuation.y;
	return attenuationFactor <= 0;
}

float BackFaceAttenuation(vec3 raySample, vec3 worldReflectionVec)
{
	// This will check the direction of the normal of the reflection sample with the
    // direction of the reflection vector, and if they are pointing in the same direction,
    // it will drown out those reflections since backward facing pixels are not available
    // for screen space reflection. Attenuate reflections for angles between 90 degrees
    // and 100 degrees, and drop all contribution beyond the (-100,100) degree range

    vec3 reflectionNormal = LoadNormal(raySample.xy);
    return smoothstep(-0.17, 0.0, dot(reflectionNormal, -worldReflectionVec));
}


in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


struct RayHit {
    float attenuationFactor;
    vec3 ssHitPosition;
};


RayHit SSReflection(vec3 worldPosition, vec3 normal);
bool RayMarch(vec3 worldReflectionVec, vec3 screenSpaceReflectionVec, vec3 screenSpacePos, inout RayHit rayHit);


// float DistSq(vec2 a, vec2 b)
// {
// 	a -= b;
// 	return dot(a, a);
// }

// void swap(inout float a, inout float b)
// {
// 	float t = a;
// 	a = b;
// 	b = t;
// }

// float LinearizeDepth(float depth) 
// {
//     float z = depth * 2.0-1.0; // back to NDC 
//     float near = exposureNearFar.y;
//     float far = exposureNearFar.z;

//     return (2.0 * near * far) / (far + near - z * (far - near));	
// }

// vec4 ViewToTextureSpace(vec3 viewSpaceVec)
// {
// 	vec4 clipSpaceVec = projection * vec4(viewSpaceVec, 1.0);
// 	//clipSpaceVec.xyz /= clipSpaceVec.w;
// 	clipSpaceVec.xy = clipSpaceVec.xy * 0.5 + 0.5;

// 	return clipSpaceVec;
// }

// bool IntersectsDepthBuffer(float z, float minZ, float maxZ)
// {
// 	float depthScale = min(1.0f, z * ssr_strideZCutoff);
// 	z += ssr_zThickness + mix(0.0f, 2.0f, depthScale);
// 	return (maxZ >= z) && (minZ - ssr_zThickness <= z);
// }

// vec3 GetViewSpacePos(vec2 uv)
// {
// 	vec3 texCoord = vec3(uv.r, uv.g, zPrePassSurface.page);
// 	float depth = texture(sampler2DArray(zPrePassSurface.container), texCoord).r;

// 	vec4 clipSpacePosition = vec4( uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
// 	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
// 	viewSpacePosition /= viewSpacePosition.w;

// 	return viewSpacePosition.xyz;
// }

void main()
{
	vec3 normal = LoadNormal(fs_in.texCoords.xy);

	vec3 worldSpacePosition = GetWorldPosition(fs_in.texCoords.xy);

	RayHit ssr = SSReflection(worldSpacePosition, normal);

	oRayHitInfo = vec4(ssr.ssHitPosition, ssr.attenuationFactor);
}

RayHit SSReflection(vec3 worldPosition, vec3 normal)
{
	vec2 pixelUV = fs_in.texCoords.xy;

	float fragDepth = SampleTextureF(zPrePassSurface, pixelUV, vec2(0));

	if(fragDepth <= 0.0 || fragDepth >= 1.0)
	{
		return RayHit(0.0, vec3(0.0));
	}

	vec3 cameraVec = normalize(worldPosition - cameraPosTimeW.xyz);

	vec4 screenSpacePos = vec4(pixelUV, fragDepth, 1.0);

	vec3 reflectionVector = reflect(cameraVec, normal);
	
	float attenuationFactor;
	if(IsReflectedBackToCamera(reflectionVector, -cameraVec, attenuationFactor))
	{
		return RayHit(0.0, vec3(0.0));
	}

	vec4 pointAlongReflectionVec = vec4(10.0f * reflectionVector + worldPosition, 1.0);
	vec4 screenSpaceReflectionPoint = vpMatrix * pointAlongReflectionVec;
	screenSpaceReflectionPoint /= screenSpaceReflectionPoint.w;
	screenSpaceReflectionPoint.xyz = screenSpaceReflectionPoint.xyz * 0.5 + 0.5;

	vec3 screenSpaceReflectionVec = normalize(screenSpaceReflectionPoint.xyz - screenSpacePos.xyz);

	RayHit rayHit;
	rayHit.ssHitPosition = vec3(0.0);
	rayHit.attenuationFactor = 0.0;

	bool rayHitDetected = RayMarch(reflectionVector, screenSpaceReflectionVec, screenSpacePos.xyz, rayHit);

	return rayHit;
}

bool RayMarch(vec3 worldReflectionVec, vec3 screenSpaceReflectionVec, vec3 screenSpacePos, inout RayHit rayHit)
{
	bool foundIntersection = false;
	vec3 minRaySample = vec3(0.0);
	vec3 maxRaySample = vec3(0.0);
	
	float viewportAttenuationFactor = 0.0;
	rayHit.attenuationFactor = 0.0;
	float previousRaySampleZ = 0.0;

	for(int rayStepIdx = 0; rayStepIdx < ssr_rayMarchIterations; rayStepIdx++)
	{
		float ditherOffset = SampleTextureF(ssr_ditherTexture, fs_in.texCoords.xy * ssr_ditherTilingFactor, vec2(0)) * 0.01  + ssr_zThickness;
		vec3 raySample = screenSpacePos + float(rayStepIdx) * ssr_maxRayMarchStep * screenSpaceReflectionVec ;
		raySample += ditherOffset * screenSpaceReflectionVec;

		if(raySample.z >= 1.0 ||
		 IsSamplingOutsideViewport(raySample, viewportAttenuationFactor))
		{
			return false;
		}

		float zBufferVal = SampleTextureF(zPrePassSurface, raySample.xy, vec2(0));

		maxRaySample = raySample;

		float bias = ssr_zThickness;

		if(raySample.z > ( zBufferVal ))
		{
			foundIntersection = true;
			break;
		}

		minRaySample = maxRaySample;
		previousRaySampleZ = raySample.z;
	}

	if(foundIntersection)
	{
		vec3 midRaySample;
		for(int i = 0; i < ssr_maxBinarySearchSamples; ++i)
		{
			midRaySample = mix(minRaySample, maxRaySample, 0.5);
			
			float zBufferVal = SampleTextureF(zPrePassSurface, midRaySample.xy, vec2(0));

			if(midRaySample.z > zBufferVal )
			{
				maxRaySample = midRaySample;
			}
			else
			{
				minRaySample = midRaySample;
			}
		}

		float backFaceAttenuationFactor = BackFaceAttenuation(midRaySample, worldReflectionVec);
		rayHit.attenuationFactor = viewportAttenuationFactor * backFaceAttenuationFactor;
		rayHit.ssHitPosition = midRaySample;
	}

	return foundIntersection;
}
