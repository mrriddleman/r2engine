#version 450 core
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_bindless_texture : require
#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 Accum;
layout (location = 1) out float Reveal;

#include "Common/MaterialInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"

in VS_OUT
{
	vec4 color;
	vec4 fragPos;
	flat uint drawID;
} fs_in;

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = projection * view * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

void main()
{	
	//float weight = TransparentWeight(fs_in.color.rgb, fs_in.color.a, computeDepth(fs_in.fragPos.xyz));

	// blend func: GL_ONE, GL_ONE
	// switch to pre-multiplied alpha and weight
    Accum = vec4(fs_in.color.rgb * fs_in.color.a, fs_in.color.a) * computeDepth(fs_in.fragPos.xyz);

    // blend func: GL_ZERO, GL_ONE_MINUS_SRC_COLOR
    Reveal = fs_in.color.a;
}