#version 410 core

struct Material 
{
    sampler2D   texture_diffuse1;
    sampler2D   texture_specular1;
    sampler2D   texture_ambient1;
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

vec4 ReflectiveEnvMap();
vec4 RefractionEnvMap(float ratio);

vec3 HalfVector(Light light, vec3 viewDir);
float CalcSpecular(vec3 lightDir, vec3 viewDir, float shininess);
float PhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess);
float BlinnPhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess);
vec3 GammaCorrect(vec3 color);

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
uniform samplerCube skybox;


float near = 0.1; 
float far  = 100.0; 

float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));    
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    for(int i = 0; i < NUM_POINT_LIGHTS; i++)
    {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }

    //result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    vec4 diffuseMat = texture(material.texture_diffuse1, TexCoord);
    //result += CalcEmissiveForMaterial(spotLight.light);

    if(diffuseMat.a < 0.01)
    {
        discard;
    }

   // float depth = LinearizeDepth(gl_FragCoord.z) / (far*0.125);
    FragColor = vec4(GammaCorrect(result), 1.0);
}

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

float CalcAttenuation(AttenuationState state, vec3 lightPos, vec3 fragPos)
{
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (distance*distance);//(state.constant + state.linear * distance + state.quadratic * (distance * distance));
    return attenuation;
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    //vec3 reflectDir = reflect(-lightDir, normal);
    float spec = BlinnPhongShading(lightDir, viewDir, normal, material.shininess);//pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
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
    //vec3 reflectDir = reflect(-lightDir, normal);
    float spec = BlinnPhongShading(lightDir, viewDir, normal, material.shininess);// pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //attenuation
    float attenuation = CalcAttenuation(light.attenuationState, light.position, fragPos);

    //combine results
    Light result = CalcLightForMaterial(light.light, diff, spec, attenuation);

    return (result.ambient + result.diffuse + result.specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDirFromFrag = normalize(light.position - fragPos);
    //diffuse shading
    float diff = max(dot(normal, lightDirFromFrag), 0.0);
    //specular shading
    //vec3 reflectDir = reflect(-lightDirFromFrag, normal);
    float spec = BlinnPhongShading(lightDirFromFrag, viewDir, normal, material.shininess);//pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    //spotlight soft edges
    float theta = dot(lightDirFromFrag, normalize(-light.direction));
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

    
    vec3 diffuseMat = texture(material.texture_diffuse1, TexCoord).rgb;
    vec3 ambientMat = diffuseMat;//vec3(texture(material.texture_ambient1, TexCoord));

    result.ambient = light.ambient * ambientMat; //* ReflectiveEnvMap().rgb;
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

vec4 ReflectiveEnvMap()
{
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I, normalize(Normal));
    return vec4(texture(skybox, R).rgb, 1.0);
}

vec4 RefractionEnvMap(float ratio)
{
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = refract(I, normalize(Normal), ratio);
    return vec4(texture(skybox, R).rgb, 1.0);
}

vec3 HalfVector(vec3 lightDir, vec3 viewDir)
{
    return normalize(normalize(lightDir) + normalize(viewDir));
}

float CalcSpecular(vec3 lightDir, vec3 viewDir, float shininess)
{   
    return pow(max(dot(viewDir, lightDir), 0.0), shininess);
}

float PhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess)
{
    vec3 reflectDir = reflect(-inLightDir, normal);
    return CalcSpecular(reflectDir, viewDir, shininess);
}

float BlinnPhongShading(vec3 inLightDir, vec3 viewDir, vec3 normal, float shininess)
{
    return CalcSpecular(HalfVector(inLightDir, viewDir), normal, shininess);
}