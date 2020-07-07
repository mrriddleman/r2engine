#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec2 TexCoord;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

//const int NUM_MODELS = 1000;

//layout (std140, binding = 1) uniform Models
//{
//	mat4 models[NUM_MODELS];
//};

uniform mat4 model;

void main()
{
	TexCoord = aTexCoord;
	Normal = aNormal;
	gl_Position = projection * view * model * vec4(aPos, 1.0);
}