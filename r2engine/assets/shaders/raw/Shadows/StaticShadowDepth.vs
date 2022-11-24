#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

void main()
{
	gl_Position = models[DrawID] * vec4(aPos, 1.0);
}