#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in uint DrawID;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

out VS_OUT
{
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.drawID = DrawID;
	gl_Position = projection * view * models[DrawID] * vec4(aPos, 1.0);
}
