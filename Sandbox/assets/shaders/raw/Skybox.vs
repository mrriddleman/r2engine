#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"

out VS_OUT
{
	out vec3 texCoords;
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.texCoords = vec3(aPos.x, aPos.z, -aPos.y); //z is up instead of y
	vs_out.drawID = DrawID;
	vec4 pos = projection * skyboxView * vec4(aPos, 1.0);
	gl_Position = pos.xyww;
}