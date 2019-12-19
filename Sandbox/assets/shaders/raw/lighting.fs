#version 410 core

struct Material 
{
    sampler2D   texture_diffuse1;
    sampler2D   texture_specular1;
    sampler2D   emission;
    float       shininess;
};

struct AttenuationState
{
    float constant;
    float linear;
    float quadratic;
};

struct Light
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emission;
};

struct DirLight
{
    vec3 direction;

    Light light;
};

struct PointLight
{
    vec3 position;

    AttenuationState attenuationState;

    Light light;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;

    Light light;

    AttenuationState attenuationState;

    float cutoff;
    float outerCutoff;
};


float CalcAttenuation(AttenuationState state, vec3 lightPos, vec3 fragPos);
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
Light CalcLightForMaterial(Light light, float diff, float spec, float modifier);
vec3 CalcEmissiveForMaterial(Light light);

out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform float time;
uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
#define NUM_POINT_LIGHTS 2
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform SpotLight spotLight;


void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    for(int i = 0; i < NUM_POINT_LIGHTS; i++)
    {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }

    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    //result += CalcEmissiveForMaterial(spotLight.light);

    FragColor = vec4(result, 1.0);
}

float CalcAttenuation(AttenuationState state, vec3 lightPos, vec3 fragPos)
{
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (state.constant + state.linear * distance + state.quadratic * (distance * distance));
    return attenuation;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //combine the results
    Light result = CalcLightForMaterial(light.light, diff, spec, 1.0);

    return result.ambient + result.diffuse + result.specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //attenuation
    float attenuation = CalcAttenuation(light.attenuationState, light.position, fragPos);

    //combine results
    Light result = CalcLightForMaterial(light.light, diff, spec, attenuation);

    return (result.ambient + result.diffuse + result.specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    //spotlight soft edges
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.cutoff - light.outerCutoff);
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

    float attenuation = CalcAttenuation(light.attenuationState, light.position, fragPos);

    //combine results
    Light result = CalcLightForMaterial(light.light, diff, spec, attenuation * intensity);

    return (result.ambient + result.diffuse + result.specular);
}

Light CalcLightForMaterial(Light light, float diff, float spec, float modifier)
{
    Light result;

    vec3 diffuseMat = vec3(texture(material.texture_diffuse1, TexCoord));

    result.ambient = light.ambient * diffuseMat;
    result.diffuse = light.diffuse * diff * diffuseMat;
    result.specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoord));

    result.ambient *= modifier;
    result.diffuse *= modifier;
    result.specular *= modifier;

    return result;
}

vec3 CalcEmissiveForMaterial(Light light)
{
    vec3 emission = vec3(0.0);
    vec3 emissionTex = texture(material.emission, TexCoord).rgb;
    vec3 specularTex = vec3(texture(material.texture_specular1, TexCoord));

    emission = emissionTex;
    emission = texture(material.emission, TexCoord + vec2(sin(time)*0.1, time)).rgb; //this moves the texture in x,y
    emission = light.emission * (floor(1.0 - specularTex.r) * emission * (sin(time) * 0.5 + 0.6) * 2.0f);//this will fade the emission in and out
    return emission;
}