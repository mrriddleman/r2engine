#ifndef GLSL_MODEL_FUNCTIONS
#define GLSL_MODEL_FUNCTIONS

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/ShaderBufferObjects/BoneTransformData.glsl"

mat4 GetStaticModel(uint drawID)
{
	//@TODO(Serge): add in the local matrix if we don't multiply it initially
	return models[drawID]; 
}

mat4 GetAnimatedModel(uint drawID)
{
	int boneOffset = boneOffsets[drawID].x;
	mat4 localMatrix = inverse(bonesXForms[BoneIDs[0] + boneOffset].globalInv);

	mat4 finalBoneVertexTransform = bonesXForms[BoneIDs[0] + boneOffset].globalInv * bonesXForms[BoneIDs[0] + boneOffset].transform * bonesXForms[BoneIDs[0] + boneOffset].invBinPose * BoneWeights[0];
	finalBoneVertexTransform 	 += bonesXForms[BoneIDs[1] + boneOffset].globalInv * bonesXForms[BoneIDs[1] + boneOffset].transform * bonesXForms[BoneIDs[1] + boneOffset].invBinPose * BoneWeights[1];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[2] + boneOffset].globalInv * bonesXForms[BoneIDs[2] + boneOffset].transform * bonesXForms[BoneIDs[2] + boneOffset].invBinPose * BoneWeights[2];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[3] + boneOffset].globalInv * bonesXForms[BoneIDs[3] + boneOffset].transform * bonesXForms[BoneIDs[3] + boneOffset].invBinPose * BoneWeights[3]; 

	return models[drawID] * localMatrix * finalBoneVertexTransform;
}

mat3 GetNormalMatrix(mat4 matrix)
{
	return transpose(inverse(mat3(matrix)));
}

#endif