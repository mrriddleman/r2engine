#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;
const float near = 0.1; 
const float far  = 100.0; 

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
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
};

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);

vec3 ReinhardToneMapping(vec3 hdrColor);
vec3 ExposureToneMapping(vec3 hdrColor);
vec3 FilmicToneMapping(vec3 color);
vec3 ACESFitted(vec3 color);
vec3 ACES(vec3 color);
vec3 Uncharted2ToneMapping(vec3 color);

float LinearizeDepth(float depth) 
{
	return depth / 100.0;
   // float z = depth * 2.0 - 1.0; // back to NDC 
   // return (2 *  near * far) / (far + near - z * (far - near));	
}

float NormalizeViewDepth(float depth)
{
	//return depth / 100.0;
	return (depth - near) / (far - near) * depth;
}

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	//vec3 toneMapping = FilmicToneMapping(sampledColor.rgb);
//	FragColor = vec4(vec3(LinearizeDepth(sampledColor.r)), 1.0);
	FragColor = vec4(ACESFitted(sampledColor.rgb), 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{

	vec3 coord = vec3(uv.r, uv.g, gBufferSurface.page );


	//vec2 ao = texture(sampler2DArray(ambientOcclusionSurface.container), coord).rg;
	//return vec4(ao.rrr, 1);



	//return texture(sampler2DArray(shadowsSurface.container), coord) * 100;
	return texture(sampler2DArray(gBufferSurface.container), coord) ;
	// highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	// Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	// vec3 coord = vec3(uv.rg,addr.page);

	// return texture(sampler2DArray(addr.container), coord);
}

vec3 ReinhardToneMapping(vec3 hdrColor)
{
	hdrColor *= exposureNearFar.x / (1.0 + hdrColor / exposureNearFar.x);
	return GammaCorrect(hdrColor);
	//return hdrColor / (hdrColor + vec3(1.0));
}

vec3 ExposureToneMapping(vec3 hdrColor)
{
	return GammaCorrect( vec3(1.0) - exp(-hdrColor * exposureNearFar.x  ));
}

vec3 FilmicToneMapping(vec3 color)
{
	vec3 X = max(vec3(0.0), color - 0.004) * exposureNearFar.x;
	return (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
}

vec3 ACES(vec3 x)
{
	const float a = 2.51;
  	const float b = 0.03;
  	const float c = 2.43;
  	const float d = 0.59;
  	const float e = 0.14;

	return GammaCorrect(clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0));
}

// The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
// credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)



// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat =
{
	{0.59719, 0.076, 0.0284},
	{0.35458, 0.90834, 0.13383},
	{0.04823, 0.01566, 0.83777}
};

// // ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
{
	{1.60475, -0.10208, -0.00327},
	{-0.53108, 1.10813, -0.07276},
	{-0.07367, -0.00605, 1.07602}
};

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
	
	color = ACESInputMat * color;
	color = RRTAndODTFit(color);
	color = ACESOutputMat * color;

	return GammaCorrect(clamp(color, 0.0, 1.0));
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	return GammaCorrect(color);
}