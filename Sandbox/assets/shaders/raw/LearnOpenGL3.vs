#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 instanceMatrix;

out vec3 Normal;
out vec3 Position;
out vec2 TexCoord;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	Normal = mat3(transpose(inverse(instanceMatrix))) * aNormal;
	gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
	TexCoord = aTexCoord;
}