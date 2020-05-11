#version 410 core
out float FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];

int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

const vec2 noiseScale = vec2(1024.0/4.0, 720.0/4.0);

uniform mat4 projection;

void main()
{
	//get input for SSAO algorithm
	vec3 fragPos = texture(gPosition, TexCoord).xyz;
	vec3 normal = normalize(texture(gNormal, TexCoord).rgb);
	vec3 randomVec = normalize(texture(texNoise, TexCoord * noiseScale).xyz);
	//create TBN change of basis matrix from tangent space to view-space
	vec3 tangent = normalize(randomVec - normal * dot(normal, randomVec));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	//iterate over the sample kernel and calculate occlusion factor
	float occlusion = 0.0;
	for(int i = 0; i < kernelSize; ++i)
	{
		vec3 aSample = TBN * samples[i];
		aSample = fragPos + aSample * radius;

		//project sample position 
		vec4 offset = vec4(aSample, 1.0);
		offset = projection * offset; // from view to clip - space
		offset.xyz /= offset.w; // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // set to range 0.0 - 1.0

		float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= aSample.z + bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / kernelSize);
	FragColor = occlusion;
}