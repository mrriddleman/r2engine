#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) out vec4 Accum;
layout (location = 1) out float Reveal;

#include "Common/MaterialInput.glsl"

in VS_OUT
{
	vec4 color;
	flat uint drawID;
} fs_in;

void main()
{	
	float weight = TransparentWeight(fs_in.color.rgb, fs_in.color.a, gl_FragCoord.z);

	// blend func: GL_ONE, GL_ONE
	// switch to pre-multiplied alpha and weight
    Accum = vec4(fs_in.color.rgb * fs_in.color.a, fs_in.color.a) * weight;

    // blend func: GL_ZERO, GL_ONE_MINUS_SRC_COLOR
    Reveal = fs_in.color.a;
}