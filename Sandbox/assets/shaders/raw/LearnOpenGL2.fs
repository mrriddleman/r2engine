#version 410 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;
uniform float near_plane;
uniform float far_plane;

struct Light
{
	vec3 Position;
	vec3 Color;

	float Linear;
	float Quadratic;

	float Radius;
};

const int NUM_LIGHTS = 32;
uniform Light lights[NUM_LIGHTS];
uniform vec3 viewPos;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

vec4 InversionPostProc();

vec4 GrayscalePostProc();
vec4 NarcoticPostProc();
vec4 GuassBlurPostProc();
vec4 BoxBlurPostProc();
vec4 EdgeDetectionPostProc();
vec4 OutlinePostProc();
vec4 KernelPostProc(float offset, float[9] kernel);
vec3 GammaCorrect(vec3 color);
vec3 ToneMap(vec3 color);
vec4 BloomPostProc();


float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

void main()
{
	//Normal
	//float depthValue = texture(screenTexture, TexCoord).r;
	//FragColor = vec4(vec3(depthValue),1.0);
	//FragColor = BloomPostProc();
	//FragColor = BoxBlurPostProc();


	vec3 FragPos = texture(gPosition, TexCoord).rgb;
	vec3 Normal = texture(gNormal, TexCoord).rgb;
	vec3 Diffuse = texture(gAlbedoSpec, TexCoord).rgb;
	float Specular = texture(gAlbedoSpec, TexCoord).a;

	vec3 lighting = Diffuse * 0.1;
	vec3 viewDir = normalize(viewPos - FragPos);
	for(int i = 0; i < NUM_LIGHTS; ++i)
	{
		float distance = length(lights[i].Position - FragPos);
		if(distance < lights[i].Radius)
		{
			//diffuse
			vec3 lightDir = normalize(lights[i].Position - FragPos);
			vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;

			//specular
			vec3 halfVec = normalize(lightDir + viewDir);
			float spec = pow(max(dot(Normal, halfVec), 0.0), 16.0);
			vec3 specular = lights[i].Color * spec * Specular;

			float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
			diffuse *= attenuation;
			specular *= attenuation;
			lighting += diffuse + specular;
		}
	}

	FragColor = vec4(lighting, 1.0);

}

vec4 InversionPostProc()
{
	//Post Process Effect - Inversion
	return vec4(vec3(1.0 - texture(screenTexture, TexCoord)), 1.0);
}

vec4 GrayscalePostProc()
{
	//Grayscale
	vec4 col = texture(screenTexture, TexCoord);
	//float average = (col.r + col.g + col.b) / 3.0;
	float average = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
	return vec4(average, average, average, 1.0);
}

vec4 NarcoticPostProc()
{
	float kernel[9] = float[](
		-1, -1, -1,
		-1,  9, -1,
		-1, -1, -1
	);

	return KernelPostProc(1.0/300.0, kernel);
}

vec4 GuassBlurPostProc()
{
	float kernel[9] = float[](
		1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
		2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
		1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
	);

	return KernelPostProc(1.0/300.0, kernel);
}

vec4 BoxBlurPostProc()
{
	float kernel[9] = float[](
		1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0,
		1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0,
		1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0
	);

	return KernelPostProc(1.0/300.0, kernel);
}

vec4 EdgeDetectionPostProc()
{
	float kernel[9] = float[](
		1, 1, 1,
		1, -8, 1,
		1, 1, 1
	);

	return KernelPostProc(1.0/300.0, kernel);
}

vec4 OutlinePostProc()
{
		float kernel[9] = float[](
		-1, -1, -1,
		-1,  8, -1,
		-1, -1, -1
	);

	return KernelPostProc(1.0/300.0, kernel);
}

vec4 KernelPostProc(float offset, float[9] kernel)
{
	vec2 offsets[9] = vec2[](
		vec2(-offset, offset), 	//top-left
		vec2(0.0f, offset), 	//top-center
		vec2(offset, offset),	//top-right
		vec2(-offset, 0.0f), 	//center-left
		vec2(0.0f, 0.0f),		//center-center
		vec2(offset, 0.0f),		//center-right
		vec2(-offset, -offset),	//bottom-left
		vec2(0.0f, -offset),	//bottom-center
		vec2(offset, -offset) 	//bottom-right
	);

	vec3 sampleTex[9];
	for(int i = 0; i < 9; i++)
	{
		sampleTex[i] = vec3(texture(screenTexture, TexCoord.st + offsets[i]));
	}

	vec3 col = vec3(0.0);
	for(int i = 0; i < 9; i++)
	{
		col += sampleTex[i] * kernel[i];
	}

	return vec4(col, 1.0);
}

vec3 GammaCorrect(vec3 color)
{
    return pow(color, vec3(1.0/2.2));
}

vec3 ToneMap(vec3 color)
{
	return vec3(1.0) - exp(-color * exposure);
}

vec4 BloomPostProc()
{
	vec3 hdrColor = texture(screenTexture, TexCoord).rgb;
	vec3 bloomColor = texture(bloomBlur, TexCoord).rgb;

	if(bloom)
		hdrColor += bloomColor;

	return vec4(GammaCorrect(ToneMap(hdrColor)), 1.0); 
}