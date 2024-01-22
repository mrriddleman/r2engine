#ifndef GLSL_ANIMATED_MODEL_FUNCTIONS
#define GLSL_ANIMATED_MODEL_FUNCTIONS

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/ShaderBufferObjects/BoneTransformData.glsl"
#include "Input/ShaderBufferObjects/MeshData.glsl"

mat4 GetAnimatedModel(uint drawID, uint localMeshIndex)
{
	int boneOffset = boneOffsets[drawID].x;

	MeshData meshData = GetMeshData(drawID, localMeshIndex);

	mat4 finalBoneVertexTransform = meshData.globalInvTransform * bonesXForms[BoneIDs[0] + boneOffset].transform * bonesXForms[BoneIDs[0] + boneOffset].invBinPose * BoneWeights[0];
	finalBoneVertexTransform 	 += meshData.globalInvTransform * bonesXForms[BoneIDs[1] + boneOffset].transform * bonesXForms[BoneIDs[1] + boneOffset].invBinPose * BoneWeights[1];
	finalBoneVertexTransform	 += meshData.globalInvTransform * bonesXForms[BoneIDs[2] + boneOffset].transform * bonesXForms[BoneIDs[2] + boneOffset].invBinPose * BoneWeights[2];
	finalBoneVertexTransform	 += meshData.globalInvTransform * bonesXForms[BoneIDs[3] + boneOffset].transform * bonesXForms[BoneIDs[3] + boneOffset].invBinPose * BoneWeights[3]; 

	return models[drawID] * meshData.globalTransform * finalBoneVertexTransform;
}

mat3 GetNormalMatrix(mat4 matrix)
{
	return transpose(inverse(mat3(matrix)));
}

#endif