#version 410 core

struct Material 
{
    sampler2D   diffuse;
    sampler2D   specular;
    sampler2D   emission;
    float       shininess;
};

struct Light
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emission;
    
    float constant;
    float linear;
    float quadratic;
    vec3 position;
    vec3 direction;
    float cutoff;
    float outerCutoff;
};

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform float time;
uniform vec3 viewPos;

uniform Material material;
uniform Light light;

void main()
{
    vec3 diffuseVec = vec3(texture(material.diffuse, TexCoord));
    vec3 ambient = diffuseVec * light.ambient;
    
    vec3 lightPos = light.position;
    vec3 lightDir = normalize(lightPos - FragPos);
    
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * diffuseVec) * light.diffuse;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specularTex = texture(material.specular, TexCoord).rgb;
    vec3 specular = specularTex * spec * light.specular;
    
    vec3 emission = vec3(0.0);
    /*
        vec3 emissionTex = texture(material.emission, TexCoord).rgb;
        emission = emissionTex;
        emission = texture(material.emission, TexCoord + vec2(sin(time)*0.1, time)).rgb; //this moves the texture in x,y
        emission = light.emission * (floor(1.0 - specularTex.r) * emission * (sin(time) * 0.5 + 0.6) * 2.0f);//this will fade the emission in and out
     */
    
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    vec3 result = (ambient + diffuse + specular + emission);
    FragColor = vec4(result, 1.0);
}