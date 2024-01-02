#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable


#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"
#include "Input/UniformBuffers/SSRParams.glsl"

//#define NUM_FRUSTUM_SPLITS 4

//https://sakibsaikia.github.io/graphics/2016/12/26/Screen-Space-Reflection-in-Killing-Floor-2.html
//http://roar11.com/2015/07/screen-space-glossy-reflections/

layout(location = 0) out vec4 oRayHitInfo;

vec3 DecodeNormal(vec2 enc)
{
	vec3 n;

	n.xy = enc * 2.0 - 1.0;
	n.z = sqrt(1.0 - dot(n.xy, n.xy));
	return normalize(n);
}

vec3 LoadNormal(vec2 uv)
{
	vec3 normal = SampleTexture(normalSurface, uv);
	return normalize(normal);// DecodeNormal(normal);
}

float SampleTextureF(Tex2DAddress tex, vec2 uv, vec2 offset)
{
	return SampleTextureR(tex, uv + offset);
}

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

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
	return (maxZ >= z) && (minZ - ssr_zThickness <= z);
}

vec3 GetViewSpacePos(vec2 uv)
{
	float depth = SampleTextureR(zPrePassSurface, uv) * 2.0 - 1.0;

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


