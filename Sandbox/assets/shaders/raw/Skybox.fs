#version 450 core

#extension GL_NV_gpu_shader5 : enable

const uint NUM_TEXTURES_PER_DRAWID = 8;

layout (location = 0) out vec4 FragColor;

struct Tex2DAddress
{
	uint64_t container;
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
	highp uint texIndex = drawID * NUM_TEXTURES_PER_DRAWID;
	Tex2DAddress addr = materials[texIndex];
	return texture(samplerCubeArray(addr.container), vec4(uv.rgb, addr.page));
} 