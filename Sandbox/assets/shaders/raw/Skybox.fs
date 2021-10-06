#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t container;
	float page;	
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
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};

in VS_OUT
{
	vec3 texCoords;
	flat uint drawID;
} fs_in;

vec4 SampleCubemapDiffuse(uint drawID, vec3 uv);

void main()
{
	vec4 sampledCubeColor = SampleCubemapDiffuse(fs_in.drawID, fs_in.texCoords);
	

	FragColor = vec4(sampledCubeColor.rgb, 1.0);
}

vec4 SampleCubemapDiffuse(uint drawID, vec3 uv) 
{
	highp uint texIndex =  drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;
	return textureLod(samplerCubeArray(addr.container), vec4(uv.rgb, addr.page), 0);
} 