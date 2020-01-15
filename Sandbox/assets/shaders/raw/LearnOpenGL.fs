#version 410 core
out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float far_plane;

// float ShadowCalculation(vec4 fragPosLightSpace)
// {
// 	//Do the clip space transformation by dividing the xyz position by w
// 	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
// 	//put projCoords into a [0,1] range so that it mimics the depth buffer (which is [0,1])
// 	projCoords = projCoords * 0.5 + 0.5; 
// 	//Get the closest depth value from the shadow map
// 	float closestDepth = texture(shadowMap, projCoords.xy).r;
// 	//Get the current depth value of this fragment in light space
// 	float currentDepth = projCoords.z;
// 	//If the currentDepth is greater than the closestDepth, then this fragment is in shdow
// 	vec3 normal = normalize(fs_in.Normal);
// 	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
// 	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

// 	float shadow = 0.0;
// 	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
// 	for(int x = -1; x <= 1; ++x)
// 	{
// 		for(int y = -1; y <= 1; ++y)
// 		{
// 			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y) * texelSize).r;
// 			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
// 		}
// 	}
// 	shadow /= 9.0;

// 	//float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0; //1.0 means we're in shadow

// 	if(projCoords.z > 1.0)
// 		shadow = 0.0;

// 	return shadow;
// }

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);   


float ShadowCalculation(vec3 fragPos)
{
	vec3 fragToLight = fragPos - lightPos;
	float currentDepth = length(fragToLight);

	

	float bias = 0.15;
	float shadow = 0.0;
	int samples = 20;
	float viewDistance = length(viewPos - fragPos);
	float diskRadius = 0.05;

	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= far_plane;
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}

	shadow /= float(samples);

//	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

void main()
{             
	vec3 color = texture(diffuseTexture, fs_in.TexCoord).rgb;
	vec3 normal = normalize(fs_in.Normal);
	vec3 lightColor = vec3(0.3);
	//ambient
	vec3 ambient = 0.3 * color;
	//diffuse
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 diffuse = diff * lightColor;
	//specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);
	vec3 halfVec = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfVec), 0.0), 64.0);
	vec3 specular = spec * lightColor;

	float shadow = ShadowCalculation(fs_in.FragPos);
	vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

	FragColor = vec4(GammaCorrect(lighting), 1.0);
	//vec3 fragToLight = fs_in.FragPos - lightPos;
	//float closestDepth = texture(shadowMap, fragToLight).r;
	//closestDepth *= far_plane;

	//FragColor = vec4(vec3(closestDepth / far_plane), 1.0);//
}
