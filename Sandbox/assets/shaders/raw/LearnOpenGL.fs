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

uniform vec3 cameraPos;
uniform Material material;
uniform samplerCube texture1;

vec4 ReflectiveEnvMap();
vec4 RefractionEnvMap(float ratio);

void main()
{             
	FragColor = vec4(vec3(texture(material.texture_diffuse1, TexCoord)), 1.0);//RefractionEnvMap(1.00 / 1.33);
}

vec4 ReflectiveEnvMap()
{
	vec3 I = normalize(Position - cameraPos);
	vec3 R = reflect(I, normalize(Normal));
   	return vec4(texture(texture1, R).rgb, 1.0);
}

vec4 RefractionEnvMap(float ratio)
{
	vec3 I = normalize(Position - cameraPos);
	vec3 R = refract(I, normalize(Normal), ratio);
	return vec4(texture(texture1, R).rgb, 1.0);
}