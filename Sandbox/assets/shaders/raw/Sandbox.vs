#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in uint DrawID;


layout (std140, binding = 0) uniform Matrices 
{
    mat4 projection;
    mat4 view;
    mat4 skyboxView;
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

layout (std140, binding = 1) uniform Vectors
{
    vec4 cameraPosTimeW;
    vec4 exposure;
};

out VS_OUT
{
	vec3 texCoords; 
	vec3 fragPos;
	vec3 normal;
	mat3 TBN;

	vec3 fragPosTangent;
	vec3 viewPosTangent;

	flat uint drawID;
} vs_out;

void main()
{
	vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	vs_out.fragPos = modelPos.xyz;
	gl_Position = projection * view * modelPos;

	mat3 normalMatrix = transpose(inverse(mat3(models[DrawID])));

	vs_out.normal = normalize(normalMatrix * aNormal);
	vec3 T = normalize(normalMatrix * aTangent);

	T = normalize(T - dot(T, vs_out.normal) * vs_out.normal);

	vec3 B = cross(vs_out.normal, T);

	


	vs_out.TBN = mat3(T, B, vs_out.normal);
	mat3 TBN = transpose(vs_out.TBN);

	vs_out.fragPosTangent = TBN * vs_out.fragPos;
	vs_out.viewPosTangent = TBN * cameraPosTimeW.xyz;

	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	
}