#version 450 core

#extension GL_NV_gpu_shader5 : enable

#define NUM_FRUSTUM_SPLITS 4

layout (location = 0) out vec2 FragColor;

struct Tex2DAddress
{
	uint64_t  container;
	float page;
	int channel;
};

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


vec2 SampleTexture(Tex2DAddress inputTexture, ivec2 uv, ivec2 uvOffset)
{
	ivec3 texCoord = ivec3(uv.x + uvOffset.x, uv.y + uvOffset.y, inputTexture.page);
	return texelFetch(sampler2DArray(inputTexture.container), texCoord, 0).rg;
}

void main(void)
{
	//@TODO(Serge): This is kinda back since we're hard coding the ao texture - would be nice to not have to do that
	//				Instead - we would need to pass a Tex2DAddress uniform 
	vec2 center = SampleTexture(ambientOcclusionSurface, ivec2(gl_FragCoord.xy), ivec2(0));
	float rampMaxInv = 1.0 / (center.y * 0.1);

	float totalAO = 0.0;
	float totalWeight = 0.0;

	for(int i = -1; i < 3 ; ++i)
	{
		for(int j = -2; j < 2; ++j)
		{
			//@TODO(Serge): This is kinda back since we're hard coding the ao texture - would be nice to not have to do that
			//				Instead - we would need to pass a Tex2DAddress uniform 
			vec2 S = SampleTexture(ambientOcclusionSurface, ivec2(gl_FragCoord.xy), ivec2(i, j));

			float weight = 1.0;//clamp(1.0 - (abs(S.y - center.y) * rampMaxInv), 0.0, 1.0);
			totalAO += S.x * weight;
			totalWeight += weight;
		}
	}

	float ao = totalAO / totalWeight;

	FragColor = vec2(ao, center.y);
}