#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in uint DrawID;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

void main()
{
	gl_Position = projection * view * models[DrawID] * vec4(aPos, 1.0);
}
