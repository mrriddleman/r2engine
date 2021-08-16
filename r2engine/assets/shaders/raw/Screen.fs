#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
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

	vec3 baseColor;
	float specular;
	float roughness;
	float metallic;
	float reflectance;
	float ambientOcclusion;
};

layout (std430, binding = 1) buffer Materials
{
	Material materials[];
};


layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposure;
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

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	vec3 toneMapping = ExposureToneMapping(sampledColor.rgb);

	FragColor = vec4(GammaCorrect(toneMapping), 1.0);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex].diffuseTexture1;

	vec3 coord = vec3(uv.rg,addr.page);

	return texture(sampler2DArray(addr.container), coord);
}

vec3 ReinhardToneMapping(vec3 hdrColor)
{
	return hdrColor / (hdrColor + vec3(1.0));
}

vec3 ExposureToneMapping(vec3 hdrColor)
{
	return vec3(1.0) - exp(-hdrColor * exposure.x  );
}