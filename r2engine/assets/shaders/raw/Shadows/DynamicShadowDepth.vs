#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/DynamicVertexInput.glsl"
#include "Common/ModelFunctions.glsl"

void main()
{
	mat4 vertexTransform = GetAnimatedModel(DrawID);
	gl_Position = vertexTransform * vec4(aPos, 1.0);
}