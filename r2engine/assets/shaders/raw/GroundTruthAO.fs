#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4
#define PI 3.141596

layout (location = 0) out vec2 FragColor;

uniform float uStrength = 1.9;
uniform float uRadius = 0.3;
uniform float uMaxRadiusPixels = 50.0;
uniform float uNumSteps = 4;


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
    mat4 invProjection;
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


//@NOTE(Serge): this is in the order of the render target surfaces in RenderTarget.h
layout (std140, binding = 2) uniform Surfaces
{
	Tex2DAddress gBufferSurface;
	Tex2DAddress shadowsSurface;
	Tex2DAddress compositeSurface;
	Tex2DAddress zPrePassSurface;
	Tex2DAddress pointLightShadowsSurface;
	Tex2DAddress ambientOcclusionSurface;
	Tex2DAddress ambientOcclusionDenoiseSurface;
};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;



vec3 GetViewSpacePos(vec2 uv)
{
	uv *= vec2(1.0 / fovAspectResXResY.z, 1.0 / fovAspectResXResY.w);

	vec3 texCoord = vec3(uv.r, uv.g, zPrePassSurface.page);
	float depth = texture(sampler2DArray(zPrePassSurface.container), texCoord).r;
	vec4 clipSpacePosition = vec4(vec3(uv, depth) * 2.0 - 1.0, 1.0);
	vec4 viewSpacePosition = invProjection * clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;

	return viewSpacePosition.xyz;
}

void ComputeSteps(inout float stepSizePix, inout float numSteps, float rayRadiusPix)
{
	numSteps = min(rayRadiusPix, uNumSteps);
	stepSizePix = rayRadiusPix / numSteps;
}

vec4 GetNoise()
{
	vec4 noise;

	ivec2 coord = ivec2(gl_FragCoord.xy);

	noise.x = (1.0 / 16.0) * ((((coord.x + coord.y) & 0x3) << 2) + (coord.x & 0x3));
	noise.z = (1.0 / 4.0) * ((coord.y - coord.x) & 0x3);

	float rotations[] = {60.0, 300.0, 180.0, 240.0, 120.0, 0.0};
	noise.y = rotations[int(frame) % 6] * (1.0 / 360.0);

	float offsets[] = {0.0, 0.5, 0.25, 0.75};
	noise.w = offsets[(int(frame) / 6) % 4];

	return noise;
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
	vec3 V1 = Pr - P;
	vec3 V2 = P - Pl;
	return (dot(V1, V1) < dot(V2, V2)) ? V1 : V2;
}

float Square(float x)
{
	return x * x;
}

float Falloff(float dist2)
{
	float start = Square(uRadius * 0.2);
	float end = Square(uRadius);
	return 2.0 * clamp((dist2 - start) / (end - start), 0.0, 1.0); 
}

void main()
{
	vec3 P = GetViewSpacePos(gl_FragCoord.xy);
	float focalLength = 1.0f / tan(fovAspectResXResY.x * 0.5f) * fovAspectResXResY.y;

	float rayRadiusUV = 0.5 * uRadius * focalLength / -P.z;
	float rayRadiusPix = rayRadiusUV * fovAspectResXResY.z;
	rayRadiusPix = min(rayRadiusPix, uMaxRadiusPixels);

	float ao = 1.0;

	if(rayRadiusPix > 1.0)
	{
		ao = 0.0;

		vec3 Pr = GetViewSpacePos(gl_FragCoord.xy + vec2(1.0, 0.0));
		vec3 Pl = GetViewSpacePos(gl_FragCoord.xy + vec2(-1.0, 0.0));
		vec3 Pt = GetViewSpacePos(gl_FragCoord.xy + vec2(0.0, 1.0));
		vec3 Pb = GetViewSpacePos(gl_FragCoord.xy + vec2(0.0, -1.0));

		vec3 dPdu = MinDiff(P, Pr, Pl);
		vec3 dPdv = MinDiff(P, Pt, Pb);

		vec3 N = normalize(cross(dPdu, dPdv));

		vec3 V = -normalize(P);

		float numSteps;
		float stepSizePix;

		ComputeSteps(stepSizePix, numSteps, rayRadiusPix);

		vec4 noise = GetNoise();

		float theta = (noise.x + noise.y) * PI;
		float jitter = noise.z + noise.w;

		vec2 dir = vec2(cos(theta), sin(theta));
		vec2 horizons = vec2(-1.0);

		float currStep = mod(jitter, 1.0) * (stepSizePix - 1.0) + 1.0;

		for(float s = 0; s < numSteps; ++s)
		{
			vec2 offset = currStep * dir;
			currStep += stepSizePix;

			//first horizon
			{
				vec3 S = GetViewSpacePos(gl_FragCoord.xy + offset); 
				vec3 D = S - P;
				float dist2 = dot(D, D);
				D *= inversesqrt(dist2);
				float attenuation = Falloff(dist2);
				horizons.x = max(dot(V, D) - attenuation, horizons.x);
			}

			//second horizon
			{
				vec3 S = GetViewSpacePos(gl_FragCoord.xy - offset);
				vec3 D = S - P;
				float dist2 = dot(D, D);
				D *= inversesqrt(dist2);
				float attenuation = Falloff(dist2);
				horizons.y = max(dot(V, D) - attenuation, horizons.y);
			}
		}

		horizons = acos(horizons);//@TODO(Serge): optimize
		horizons.x = -horizons.x;

		// project normal onto slice plane
		vec3 planeN = normalize(cross(vec3(dir, 0.0), V));
		vec3 projectedN = N - dot(N, planeN) * planeN;

		float projectedNLength = length(projectedN);
		float invLength = 1.0 / (projectedNLength + 1e-6);
		projectedN *= invLength;

		//calculate gamma
		vec3 tangent = cross(V, planeN);
		float cosGamma = dot(projectedN, V);
		float gamma = acos(cosGamma) * sign(-dot(projectedN, tangent));
		float sinGamma2 = 2.0 * sin(gamma);

		//clamp horizons
		horizons.x = gamma + max(horizons.x - gamma, -PI * 0.5);
		horizons.y = gamma + min(horizons.y - gamma, PI * 0.5);

		vec2 horizonCosTerm = (sinGamma2 * horizons - cos(2.0 * horizons - gamma)) + cosGamma;

		projectedNLength *= 0.25;

		ao += projectedNLength * horizonCosTerm.x;
		ao += projectedNLength * horizonCosTerm.y;
	}

	FragColor = vec2(ao, -P.z);
}