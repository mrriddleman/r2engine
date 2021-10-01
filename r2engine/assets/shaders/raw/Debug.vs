#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in uint DrawID;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
};

struct DebugRenderConstants
{
	vec4 color;
	mat4 modelMatrix;
};

layout (std430, binding = 5) buffer DebugConstants
{
	DebugRenderConstants constants[];
};

out VS_OUT
{
	vec4 color;
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.drawID = DrawID;
	vs_out.color = constants[DrawID].color;
	gl_Position = projection * view * constants[DrawID].modelMatrix * vec4(aPos, 1.0);
}
