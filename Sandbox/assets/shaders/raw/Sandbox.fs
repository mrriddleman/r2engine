#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

struct ObjectMaterial
{
	Tex2DAddress textureDiffuse1;
	Tex2DAddress textureSpecular1;
	Tex2DAddress textureEmissive1;
	Tex2DAddress textureNormal1;
	Tex2DAddress textureMetallic1;
	Tex2DAddress textureHeight1;
	Tex2DAddress textureMicrofacet1;
	Tex2DAddress textureOcclusion1;
};

layout (std430, binding = 1) buffer Materials
{
	ObjectMaterial materials[];
};


in VS_OUT
{
	vec3 normal;
	vec2 texCoords;
	flat uint drawID;
} fs_in;


vec4 SampleMaterialDiffuse(uint drawID, vec2 uv);

void main()
{
	FragColor = vec4(SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords).rgb, 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec2 uv)
{
	Tex2DAddress addr = materials[drawID].textureDiffuse1;
	return texture(sampler2DArray(addr.container), vec3(uv,addr.page));
}