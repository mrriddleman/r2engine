#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"

void main()
{
	gl_Position = models[DrawID] * vec4(aPos, 1.0);
}