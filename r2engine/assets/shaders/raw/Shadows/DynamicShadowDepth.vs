#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec4 BoneWeights;
layout (location = 5) in ivec4 BoneIDs;
layout (location = 6) in uint DrawID;

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/ShaderBufferObjects/BoneTransformData.glsl"

void main()
{
	int boneOffset = boneOffsets[DrawID].x;
	mat4 finalBoneVertexTransform = bonesXForms[BoneIDs[0] + boneOffset].globalInv * bonesXForms[BoneIDs[0] + boneOffset].transform * bonesXForms[BoneIDs[0] + boneOffset].invBinPose * BoneWeights[0];
	finalBoneVertexTransform 	 += bonesXForms[BoneIDs[1] + boneOffset].globalInv * bonesXForms[BoneIDs[1] + boneOffset].transform * bonesXForms[BoneIDs[1] + boneOffset].invBinPose * BoneWeights[1];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[2] + boneOffset].globalInv * bonesXForms[BoneIDs[2] + boneOffset].transform * bonesXForms[BoneIDs[2] + boneOffset].invBinPose * BoneWeights[2];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[3] + boneOffset].globalInv * bonesXForms[BoneIDs[3] + boneOffset].transform * bonesXForms[BoneIDs[3] + boneOffset].invBinPose * BoneWeights[3]; 

	mat4 vertexTransform = models[DrawID] * finalBoneVertexTransform;
	gl_Position = vertexTransform * vec4(aPos, 1.0);
}