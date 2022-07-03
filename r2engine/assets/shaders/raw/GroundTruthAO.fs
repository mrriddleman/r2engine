#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec2 FragColor;


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
    vec4 fovAspect;
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

void main()
{
	FragColor = vec2(0, 1);
}