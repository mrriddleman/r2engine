#version 450 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in uint DrawID;

layout (std140, binding = 0) buffer Models
{
	mat4 models[];	
};

out VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} vs_out;

void main()
{
	vec4 pos = models[DrawID] * vec4(aPosition, 1);
	gl_Position = vec4(pos.x, pos.y, 0, 1.0);

	vs_out.normal = aNormal;
	vs_out.texCoords = vec3(aTexCoord.x, aTexCoord.y, 0.0);
	vs_out.drawID = DrawID;
}