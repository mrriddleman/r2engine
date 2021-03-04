#version 450 core

layout (location = 0) out vec4 FragColor;

layout (std430, binding = 5) buffer Colors
{
	vec4 colors[];
};

in VS_OUT
{
	flat uint drawID;
} fs_in;


void main()
{
	FragColor = colors[fs_in.drawID];
}
