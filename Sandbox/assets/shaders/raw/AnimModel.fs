#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

const uint NUM_TEXTURES_PER_DRAWID = 8;

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


vec4 SampleMaterialDiffuse(uint drawID, vec3 uv);

void main()
{
	vec4 sampledColor = SampleMaterialDiffuse(fs_in.drawID, fs_in.texCoords);

	FragColor = vec4(sampledColor.rgb, 1.0);
	if(sampledColor.a < 0.01)
		discard;
}

vec4 SampleMaterialDiffuse(uint drawID, vec3 uv)
{
	highp uint texIndex = uint(round(uv.z)) + drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex];
	return texture(sampler2DArray(addr.container), vec3(uv.rg,addr.page));
}