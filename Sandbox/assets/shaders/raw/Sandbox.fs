#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
};

layout (std430, binding = 1) buffer Materials
{
	Tex2DAddress materials[];
};


in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

const float PI = 3.14159;
vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	FragColor = sampledColor * (0.4 * sin(cameraPosTimeW.w+PI*1.5) + 0.6);
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex];

	vec3 coord = vec3(uv.rg,addr.page);

	float mipmapLevel = textureQueryLod(sampler2DArray(addr.container), uv.rg).x;

	return textureLod(sampler2DArray(addr.container), coord, mipmapLevel);
}