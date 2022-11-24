#version 450 core

layout (location = 0) out vec4 FragColor;

in VS_OUT
{
	vec4 color;
	flat uint drawID;
} fs_in;


void main()
{
	FragColor = fs_in.color;
}
