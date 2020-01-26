#version 410 core
out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec2 TexCoord;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D normalMap;

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

void main()
{             
	vec3 color = texture(diffuseTexture, fs_in.TexCoord).rgb;
	vec3 normal = texture(normalMap, fs_in.TexCoord).rgb;
	normal = normalize(normal * 2.0 - 1.0);

	vec3 lightColor = vec3(0.3);
	//ambient
	vec3 ambient = lightColor * color;
	//diffuse
	vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * color;
	//specular
	vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
	vec3 halfVec = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfVec), 0.0), 64.0);
	vec3 specular = spec * lightColor;
	
	vec3 lighting = (ambient + diffuse + specular);

	FragColor = vec4(GammaCorrect(lighting), 1.0);

}
