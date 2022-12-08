#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;

#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/DebugData.glsl"

out VS_OUT
{
	vec4 color;
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.color = constants[DrawID].color;
	vs_out.drawID = DrawID;
	gl_Position = projection * view * constants[DrawID].modelMatrix * vec4(aPos, 1.0);
}
