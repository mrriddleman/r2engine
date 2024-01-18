#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec4 BoneWeights;
layout (location = 5) in ivec4 BoneIDs;
layout (location = 6) in uint DrawID;

//#define NUM_FRUSTUM_SPLITS 4 //TODO(Serge): pass in
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/ShaderBufferObjects/BoneTransformData.glsl"

out VS_OUT
{
	vec3 texCoords; 
	vec3 fragPos;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	mat3 TBN;

	vec3 fragPosTangent;
	vec3 viewPosTangent;

	vec3 viewNormal;

	flat uint drawID;
} vs_out;

invariant gl_Position;

void main()
{
	int boneOffset = boneOffsets[DrawID].x;
	mat4 localMatrix = inverse(bonesXForms[BoneIDs[0] + boneOffset].globalInv);

	mat4 finalBoneVertexTransform = bonesXForms[BoneIDs[0] + boneOffset].globalInv * bonesXForms[BoneIDs[0] + boneOffset].transform * bonesXForms[BoneIDs[0] + boneOffset].invBinPose * BoneWeights[0];
	finalBoneVertexTransform 	 += bonesXForms[BoneIDs[1] + boneOffset].globalInv * bonesXForms[BoneIDs[1] + boneOffset].transform * bonesXForms[BoneIDs[1] + boneOffset].invBinPose * BoneWeights[1];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[2] + boneOffset].globalInv * bonesXForms[BoneIDs[2] + boneOffset].transform * bonesXForms[BoneIDs[2] + boneOffset].invBinPose * BoneWeights[2];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[3] + boneOffset].globalInv * bonesXForms[BoneIDs[3] + boneOffset].transform * bonesXForms[BoneIDs[3] + boneOffset].invBinPose * BoneWeights[3]; 

	mat4 vertexTransform = models[DrawID] * localMatrix * finalBoneVertexTransform;
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0);

	mat3 normalMatrix = transpose(inverse(mat3(vertexTransform)));

	vs_out.normal = normalize(normalMatrix * aNormal);
	mat3 viewNormalMatrix = transpose(inverse(mat3(view * models[DrawID])));
	vs_out.viewNormal = normalize(viewNormalMatrix * aNormal);

//	vs_out.worldNormal = (vertexTransform * vec4(aNormal, 0));

	vec3 T = normalize(normalMatrix * aTangent);

	T = normalize(T - dot(T, vs_out.normal) * vs_out.normal);

	vec3 B = cross(vs_out.normal, T);

	vs_out.tangent = T;
	vs_out.bitangent = B;

	vs_out.TBN = mat3(T, B, vs_out.normal);
	mat3 TBN = transpose(vs_out.TBN);

	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	vs_out.fragPos = modelPos.xyz;

	vs_out.fragPosTangent = TBN * vs_out.fragPos;
	vs_out.viewPosTangent = TBN * cameraPosTimeW.xyz;

	gl_Position = projection * view * modelPos;

}