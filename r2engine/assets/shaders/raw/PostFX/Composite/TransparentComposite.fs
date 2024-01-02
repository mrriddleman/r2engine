#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 FragColor;

#include "Common/CommonFunctions.glsl"
#include "Common/Texture.glsl"
#include "Input/UniformBuffers/Surfaces.glsl"

in VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} fs_in;

void main()
{	
	float reveal = TexelFetch(transparentRevealSurface, ivec2(gl_FragCoord.xy), 0).r;

	if(IsApproximatelyEqual(reveal, 1.0f))
	{
		discard;
	}

	vec4 accum = TexelFetch(transparentAccumSurface, ivec2(gl_FragCoord.xy), 0);

	if(isinf(Max3(abs(accum.rgb))))
	{
		accum.rgb = vec3(accum.a);
	}

	vec3 average_color = accum.rgb / max(accum.a, EPSILON);

	FragColor = vec4(average_color, 1.0 - reveal);
}