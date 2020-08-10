#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in uint DrawID;


layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};


out VS_OUT
{
	vec3 normal;
	vec2 texCoords;
	flat uint drawID;
} vs_out;

void main()
{
	gl_Position = projection * view * models[DrawID] * vec4(aPos, 1.0);
	vs_out.normal = aNormal;
	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
}