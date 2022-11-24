#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

//https://sakibsaikia.github.io/graphics/2016/12/26/Screen-Space-Reflection-in-Killing-Floor-2.html
//http://roar11.com/2015/07/screen-space-glossy-reflections/

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
	Tex2DAddress zPrePassShadowsSurface[2];
	Tex2DAddress ambientOcclusionTemporalDenoiseSurface[2]; //current in 0
	Tex2DAddress normalSurface;
	Tex2DAddress specularSurface;
	Tex2DAddress ssrSurface;
	Tex2DAddress convolvedGBUfferSurface[2];
	Tex2DAddress ssrConeTracedSurface;
	Tex2DAddress bloomDownSampledSurface;
	Tex2DAddress bloomBlurSurface;
	Tex2DAddress bloomUpSampledSurface;
};


layout (std140, binding = 4) uniform SSRParams
{
	float ssr_stride;
	float ssr_zThickness;
	int ssr_rayMarchIterations;
	float ssr_strideZCutoff;
	
	Tex2DAddress ssr_ditherTexture;
	
	float ssr_ditherTilingFactor;
	int ssr_roughnessMips;
	int ssr_coneTracingSteps;
	float ssr_maxFadeDistance;

	float ssr_fadeScreenStart;
	float ssr_fadeScreenEnd;
	float ssr_maxDistance;
};

vec3 DecodeNormal(vec2 enc)
{
	vec3 n;

	n.xy = enc * 2.0 - 1.0;
	n.z = sqrt(1.0 - dot(n.xy, n.xy));
	return normalize(n);
}

vec3 LoadNormal(vec2 uv)
{
	vec3 coord = vec3(uv.r, uv.g, normalSurface.page );
	vec3 normal = texture(sampler2DArray(normalSurface.container), coord).rgb;

	return normalize(normal);// DecodeNormal(normal);
}

float SampleTextureF(Tex2DAddress tex, vec2 uv, vec2 offset)
{
	vec3 texCoord = vec3(uv + offset, tex.page);
	return texture(sampler2DArray(tex.container), texCoord).r;
}

// vec3 GetWorldPosition(vec2 uv)
// {
// 	float depth = SampleTextureF(zPrePassSurface, uv, vec2(0)) * 2.0 - 1.0;

// 	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

// 	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;

// 	viewSpacePosition /= viewSpacePosition.w;

// 	vec4 worldSpacePosition = inverseView * viewSpacePosition;

// 	return worldSpacePosition.xyz;
// }

// bool IsReflectedBackToCamera(vec3 reflection, vec3 fragToCamera, inout float attenuationFactor)
// {
// 	// This will check the direction of the reflection vector with the view direction,
//     // and if they are pointing in the same direction, it will drown out those reflections
//     // since we are limited to pixels visible on screen. Attenuate reflections for angles between
//     // 60 degrees and 75 degrees, and drop all contribution beyond the (-60,60)  degree range

// 	attenuationFactor = 1.0f - smoothstep(0.25f, 0.5f, dot(fragToCamera, reflection));

// 	return attenuationFactor <= 0;
// }

// bool IsSamplingOutsideViewport(vec3 raySample, inout float attenuationFactor)
// {
// 	// Any rays that march outside the screen viewport will not have any valid pixel information. These need to be dropped.
// 	vec2 uvSamplingAttenuation = smoothstep(0.0f, 0.05f, raySample.xy) * (1.0f - smoothstep(0.95f, 1.0f, raySample.xy));
// 	attenuationFactor = uvSamplingAttenuation.x * uvSamplingAttenuation.y;
// 	return attenuationFactor <= 0;
// }

// float BackFaceAttenuation(vec3 raySample, vec3 worldReflectionVec)
// {
// 	// This will check the direction of the normal of the reflection sample with the
//     // direction of the reflection vector, and if they are pointing in the same direction,
//     // it will drown out those reflections since backward facing pixels are not available
//     // for screen space reflection. Attenuate reflections for angles between 90 degrees
//     // and 100 degrees, and drop all contribution beyond the (-100,100) degree range

//     vec3 reflectionNormal = LoadNormal(raySample.xy);
//     return smoothstep(-0.17, 0.0, dot(reflectionNormal, -worldReflectionVec));
// }


in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;


// struct RayHit {
//     float attenuationFactor;
//     vec3 ssHitPosition;
// };


//RayHit SSReflection(vec3 worldPosition, vec3 normal);
//bool RayMarch(vec3 worldReflectionVec, vec3 screenSpaceReflectionVec, vec3 screenSpacePos, inout RayHit rayHit);


float DistSq(vec2 a, vec2 b)
{
	a -= b;
	return dot(a, a);
}

void swap(inout float a, inout float b)
{
	float t = a;
	a = b;
	b = t;
}

vec4 ViewToTextureSpace(vec3 viewSpaceVec)
{
	mat4 texScaleBias = mat4(1.0);
	texScaleBias[0][0] = 0.5;
	texScaleBias[1][1] = 0.5;
	texScaleBias[3][0] = 0.5;
	texScaleBias[3][1] = 0.5;

	vec4 textureVex = texScaleBias * projection * vec4(viewSpaceVec, 1.0);

	return textureVex;
}

bool IntersectsDepthBuffer(float z, float minZ, float maxZ)
{
	float depthScale = min(1.0f, z / ssr_strideZCutoff);
	z += ssr_zThickness + mix(0.0f, 2.0f, depthScale);
	return (maxZ >= z) && (minZ - ssr_zThickness<= z);
}

vec3 GetViewSpacePos(vec2 uv)
{
	vec3 texCoord = vec3(uv.r, uv.g, zPrePassSurface.page);
	float depth = texture(sampler2DArray(zPrePassSurface.container), texCoord).r *2.0 - 1.0;

	vec4 clipSpacePosition = vec4( uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;

	return viewSpacePosition.xyz;
}

float LinearDepthTexelFetch(vec2 hitPixel)
{
	vec2 uv = vec2(hitPixel) / vec2(fovAspectResXResY.zw);

	return GetViewSpacePos(uv).z;
}

bool TraceScreenSpaceRay(vec3 vsOrig, vec3 vsDir, float jitter, out vec2 hitPixel, out vec3 hitPoint);

void main()
{
	vec3 normal = LoadNormal(fs_in.texCoords.xy);
	
	vec3 vsRayOrigin = GetViewSpacePos(fs_in.texCoords.xy);
	vec3 toPositionVS = normalize(vsRayOrigin);
	vec3 rayDirectionVS = normalize(reflect(toPositionVS, normalize(normal)));

	float rDotV = dot(rayDirectionVS, toPositionVS);

	vec2 hitPixel = vec2(0);
	vec3 hitPoint = vec3(0);

	vec2 uv2 = fs_in.texCoords.xy * fovAspectResXResY.zw;
	float jitter = mod((uv2.x + uv2.y) * 0.25, 1.0);

	bool intersection = TraceScreenSpaceRay(vsRayOrigin, rayDirectionVS, jitter, hitPixel, hitPoint);

	vec2 hitPixelUV = vec2(hitPixel) / fovAspectResXResY.zw;

	float depth = SampleTextureF(zPrePassSurface, hitPixelUV, vec2(0));

	if(hitPixelUV.x > 1.0f || hitPixelUV.x < 0.0f || hitPixelUV.y > 1.0f || hitPixelUV.y < 0.0f)
    {
        intersection = false;
    }
	
	oRayHitInfo = intersection ? vec4(hitPixelUV, depth, rDotV) : vec4(0);

	//vec3 worldSpacePosition = GetWorldPosition(fs_in.texCoords.xy);
	//RayHit ssr = SSReflection(worldSpacePosition, normal);

	//oRayHitInfo =   vec4(ssr.ssHitPosition, ssr.attenuationFactor);
}

bool TraceScreenSpaceRay(vec3 vsOrig, vec3 vsDir, float jitter, out vec2 hitPixel, out vec3 hitPoint)
{
	float rayLength = ((vsOrig.z + vsDir.z * ssr_maxDistance) > -exposureNearFar.y) ? (-exposureNearFar.y - vsOrig.z) / vsDir.z : ssr_maxDistance;

	vec3 vsEndPoint = vsOrig + (vsDir) * rayLength;

	vec4 H0 = ViewToTextureSpace(vsOrig);
	H0.xy *= fovAspectResXResY.zw;

	vec4 H1 = ViewToTextureSpace(vsEndPoint);
	H1.xy *= fovAspectResXResY.zw;

	float k0 = 1.0f / H0.w;
	float k1 = 1.0f / H1.w;

	vec3 Q0 = vsOrig * k0;
	vec3 Q1 = vsEndPoint * k1;

	vec2 P0 = H0.xy * k0;
	vec2 P1 = H1.xy * k1;

	P1 += (DistSq(P0, P1) < 0.0001f) ? vec2(0.01f) : vec2(0.0);
	vec2 delta = (P1 - P0);

	bool permute = false;

	if(abs(delta.x) < abs(delta.y))
	{
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;

	vec3 dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	vec2 dP = vec2(stepDir, delta.y * invdx);

	float strideScale = 1.0f - min(1.0f, vsOrig.z / ssr_strideZCutoff);
	float stride = (1.0f + strideScale * ssr_stride);

	dP *= stride;
	dQ *= stride;
	dk *= stride;

	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	vec4 PQk = vec4(P0, Q0.z, k0);
	vec4 dPQk = vec4(dP, dQ.z, dk);
	vec3 Q = Q0;

	float end = P1.x * stepDir;

	float stepCount = 0.0f;
	float prevZMaxEstimate = vsOrig.z;
	float rayZMin = prevZMaxEstimate;
	float rayZMax = prevZMaxEstimate;

	float sceneZMax = rayZMax + 1e4;

	for(; ((PQk.x * stepDir) <= end) && (stepCount < ssr_rayMarchIterations) &&
	 !IntersectsDepthBuffer(sceneZMax, rayZMin, rayZMax) &&
	 (sceneZMax != 0.0f);
	 stepCount += 1.0)
	 {
	 	hitPixel = permute ? PQk.yx : PQk.xy;
	 	float ditherOffset = SampleTextureF(ssr_ditherTexture, hitPixel * ssr_ditherTilingFactor, vec2(0));

	 	rayZMin = prevZMaxEstimate;
	 	rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
	 	rayZMax += ditherOffset * 4;

	 	prevZMaxEstimate = rayZMax;

	 	if(rayZMin > rayZMax)
	 	{
	 		swap(rayZMin, rayZMax);
	 	}

	 	sceneZMax = LinearDepthTexelFetch(hitPixel) ;

	 	PQk += dPQk;
	 } 

	 Q.xy += dQ.xy * stepCount;
	 hitPoint = Q * (1.0f / PQk.w);
	 return IntersectsDepthBuffer(sceneZMax, rayZMin, rayZMax);
}

// RayHit SSReflection(vec3 worldPosition, vec3 normal)
// {
// 	vec2 pixelUV = fs_in.texCoords.xy;

// 	float fragDepth = SampleTextureF(zPrePassSurface, pixelUV, vec2(0));

// 	if(fragDepth <= 0.0 || fragDepth >= 1.0)
// 	{
// 		return RayHit(0.0, vec3(0.0));
// 	}

// 	vec3 cameraVec = normalize(worldPosition - cameraPosTimeW.xyz);

// 	vec4 screenSpacePos = vec4(pixelUV, fragDepth, 1.0);

// 	vec3 reflectionVector = reflect(cameraVec, normal);
	
// 	float attenuationFactor;
// 	if(IsReflectedBackToCamera(reflectionVector, -cameraVec, attenuationFactor))
// 	{
// 		return RayHit(0.0, vec3(0.0));
// 	}

// 	vec4 pointAlongReflectionVec = vec4(10.0f * reflectionVector + worldPosition, 1.0);
// 	vec4 screenSpaceReflectionPoint = vpMatrix * pointAlongReflectionVec;
// 	screenSpaceReflectionPoint /= screenSpaceReflectionPoint.w;
// 	screenSpaceReflectionPoint.xyz = screenSpaceReflectionPoint.xyz * 0.5 + 0.5;

// 	vec3 screenSpaceReflectionVec = normalize(screenSpaceReflectionPoint.xyz - screenSpacePos.xyz);

// 	RayHit rayHit;
// 	rayHit.ssHitPosition = vec3(0.0);
// 	rayHit.attenuationFactor = 0.0;

// 	bool rayHitDetected = RayMarch(reflectionVector, screenSpaceReflectionVec, screenSpacePos.xyz, rayHit);

// 	return rayHit;
// }

// bool RayMarch(vec3 worldReflectionVec, vec3 screenSpaceReflectionVec, vec3 screenSpacePos, inout RayHit rayHit)
// {
// 	bool foundIntersection = false;
// 	vec3 minRaySample = vec3(0.0);
// 	vec3 maxRaySample = vec3(0.0);
	
// 	float viewportAttenuationFactor = 0.0;
// 	rayHit.attenuationFactor = 0.0;
// 	float previousRaySampleZ = 0.0;

// 	for(int rayStepIdx = 0; rayStepIdx < ssr_rayMarchIterations; rayStepIdx++)
// 	{
// 		float ditherOffset = SampleTextureF(ssr_ditherTexture, fs_in.texCoords.xy * ssr_ditherTilingFactor, vec2(0)) * 0.01;
// 		vec3 raySample = screenSpacePos + float(rayStepIdx) * ssr_maxRayMarchStep * screenSpaceReflectionVec ;
// 		raySample += ditherOffset * screenSpaceReflectionVec ;

// 		if(raySample.z >= 1.0 ||
// 		 IsSamplingOutsideViewport(raySample, viewportAttenuationFactor))
// 		{
// 			return false;
// 		}

// 		float zBufferVal = SampleTextureF(zPrePassSurface, raySample.xy, vec2(0));

// 		maxRaySample = raySample;

// 		float bias = ssr_zThickness;

// 		if((raySample.z ) > ( zBufferVal + bias))
// 		{
// 			foundIntersection = true;
// 			break;
// 		}

// 		minRaySample = maxRaySample;
// 		previousRaySampleZ = raySample.z;
// 	}

// 	if(foundIntersection)
// 	{
// 		vec3 midRaySample;
// 		for(int i = 0; i < ssr_maxBinarySearchSamples; ++i)
// 		{
// 			midRaySample = mix(minRaySample, maxRaySample, 0.5);
			
// 			float zBufferVal = SampleTextureF(zPrePassSurface, midRaySample.xy, vec2(0));

// 			if(midRaySample.z > zBufferVal)
// 			{
// 				maxRaySample = midRaySample;
// 			}
// 			else
// 			{
// 				minRaySample = midRaySample;
// 			}
// 		}

// 		float backFaceAttenuationFactor = BackFaceAttenuation(midRaySample, worldReflectionVec);
// 		rayHit.attenuationFactor = viewportAttenuationFactor * backFaceAttenuationFactor;
// 		rayHit.ssHitPosition = midRaySample;
// 	}

// 	return foundIntersection;
// }


