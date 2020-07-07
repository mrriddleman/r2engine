#version 450 core

struct Material
{
	sampler2D 	texture_diffuse1;
	sampler2D 	texture_specular1;
	sampler2D	texture_emissive1;
	sampler2D	texture_normal1;
	sampler2D	texture_metallic1;
	sampler2D	texture_height1;
	sampler2D	texture_microfacet1;
	sampler2D	texture_occlusion1;

	vec4 		color;
};

uniform Material material;

out vec4 FragColor;
in vec3 Normal;
in vec2 TexCoord;

void main()
{
	FragColor = vec4(texture(material.texture_diffuse1, TexCoord).rgb, 1.0);
}