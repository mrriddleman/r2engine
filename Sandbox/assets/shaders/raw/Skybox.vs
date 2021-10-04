#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
};

out VS_OUT
{
	out vec3 texCoords;
	flat uint drawID;
} vs_out;

void main()
{
	vs_out.texCoords = aPos;
	vs_out.drawID = DrawID;
	vec4 pos = projection * skyboxView * vec4(aPos, 1.0);
	gl_Position = pos.xyww;
}