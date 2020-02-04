#version 410 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
} fs_in;

struct Material
{
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
};

uniform Material material;

void main()
{
	gPosition = fs_in.FragPos;
	gNormal = normalize(fs_in.Normal);
	gAlbedoSpec.rgb = texture(material.texture_diffuse1, fs_in.TexCoord).rgb;
	gAlbedoSpec.a = texture(material.texture_specular1, fs_in.TexCoord).r;
}
