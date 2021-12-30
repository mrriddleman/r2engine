#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;
const float near = 0.1; 
const float far  = 500.0; 

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposure;
    vec4 cascadePlanes;
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

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2 *  near * far) / (far + near - z * (far - near));	
}

float NormalizeViewDepth(float depth)
{
	return (depth - near) / (far - near) * depth;
}

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	vec3 toneMapping = ExposureToneMapping(sampledColor.rgb);
	//FragColor = vec4(vec3(LinearizeDepth(sampledColor.r)), 1.0);
	FragColor = vec4(GammaCorrect(toneMapping), 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{

	vec3 coord = vec3(uv.r, uv.g, gBufferSurface.page);

//	return texture(sampler2DArray(shadowsSurface.container), coord);
	return texture(sampler2DArray(gBufferSurface.container), coord);
	// highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	// Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	// vec3 coord = vec3(uv.rg,addr.page);

	// return texture(sampler2DArray(addr.container), coord);
}

vec3 ReinhardToneMapping(vec3 hdrColor)
{
	return hdrColor / (hdrColor + vec3(1.0));
}

vec3 ExposureToneMapping(vec3 hdrColor)
{
	return vec3(1.0) - exp(-hdrColor * exposure.x  );
}