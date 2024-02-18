#ifndef GLSL_ANIMATED_MODEL_FUNCTIONS
#define GLSL_ANIMATED_MODEL_FUNCTIONS

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/ShaderBufferObjects/BoneTransformData.glsl"
#include "Input/ShaderBufferObjects/MeshData.glsl"

mat4 GetAnimatedModel(uint drawID, uint localMeshIndex)
{
	int boneOffset = boneOffsets[drawID].x;

	MeshData meshData = GetMeshData(drawID, localMeshIndex);

	BoneTransform boneXForm0 = bonesXForms[BoneIDs[0] + boneOffset];
	BoneTransform boneXForm1 = bonesXForms[BoneIDs[1] + boneOffset];
	BoneTransform boneXForm2 = bonesXForms[BoneIDs[2] + boneOffset];
	BoneTransform boneXForm3 = bonesXForms[BoneIDs[3] + boneOffset];

	mat4 finalBoneVertexTransform =  boneXForm0.transform * boneXForm0.invBinPose * BoneWeights[0];
	finalBoneVertexTransform 	 +=  boneXForm1.transform * boneXForm1.invBinPose * BoneWeights[1];
	finalBoneVertexTransform	 +=  boneXForm2.transform * boneXForm2.invBinPose * BoneWeights[2];
	finalBoneVertexTransform	 +=  boneXForm3.transform * boneXForm3.invBinPose * BoneWeights[3]; 

	return models[drawID] * meshData.globalTransform * meshData.globalInvTransform * finalBoneVertexTransform;
}

mat3 GetNormalMatrix(mat4 matrix)
{
	return transpose(inverse(mat3(matrix)));
}

#endif