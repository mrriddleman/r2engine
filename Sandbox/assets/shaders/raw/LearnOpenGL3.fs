#version 410 core

out vec4 FragColor;

in vec3 Normal;
in vec3 Position;
in vec2 TexCoord;

struct Material 
{
    sampler2D   texture_diffuse1;
    sampler2D   texture_specular1;
    sampler2D	texture_ambient1;
};

uniform Material material;

void main()
{             
	FragColor = vec4(vec3(texture(material.texture_diffuse1, TexCoord)), 1.0);
}