#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/DebugVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/DebugData.glsl"

out VS_OUT
{
	vec4 color;
	vec4 fragPos;
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.drawID = DrawID;
	vs_out.color = constants[DrawID].color;
	vs_out.fragPos = constants[DrawID].modelMatrix * vec4(aPos, 1.0);
	gl_Position = projection * view * vs_out.fragPos;
}
