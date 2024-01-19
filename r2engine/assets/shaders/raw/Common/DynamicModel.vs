#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/DynamicVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Common/ModelFunctions.glsl"

invariant gl_Position;

out VS_OUT
{
	flat uint drawID;
	vec3 texCoords; 
} vs_out;


void main()
{
	
	mat4 vertexTransform = GetAnimatedModel(DrawID);
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0);
	
	gl_Position = projection * view * modelPos;

	vs_out.drawID = DrawID;
	vs_out.texCoords= aTexCoord;
}