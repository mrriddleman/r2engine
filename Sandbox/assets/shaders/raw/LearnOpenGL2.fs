#version 410 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D screenTexture;

vec4 InversionPostProc();
vec4 GrayscalePostProc();
vec4 NarcoticPostProc();
vec4 BlurPostProc();
vec4 EdgeDetectionPostProc();
vec4 KernelPostProc(float offset, float[9] kernel);

void main()
{
	//Normal
	//FragColor = vec4(vec3(texture(screenTexture, TexCoord)), 1.0);

	FragColor = EdgeDetectionPostProc();
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

vec4 BlurPostProc()
{
	float kernel[9] = float[](
		1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
		2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
		1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
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