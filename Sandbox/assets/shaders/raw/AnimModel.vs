#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in float aTangent;
layout (location = 4) in vec3 aBiTangent;
layout (location = 5) in vec4 BoneWeights;
layout (location = 6) in ivec4 BoneIDs;
layout (location = 7) in uint DrawID;

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

struct BoneTransform
{
	mat4 globalInv;
	mat4 transform;
	mat4 invBinPose;
};

layout (std430, binding = 2) buffer BoneTransforms
{
	BoneTransform bonesXForms[];
};

layout (std140, binding = 3) buffer BoneTransformOffsets
{
	ivec4 boneOffsets[];
};

out VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	vec3 fragPos;

	flat uint drawID;
} vs_out;

void main()
{
	int boneOffset = boneOffsets[DrawID].x;
	mat4 finalBoneVertexTransform = bonesXForms[BoneIDs[0] + boneOffset].globalInv * bonesXForms[BoneIDs[0] + boneOffset].transform * bonesXForms[BoneIDs[0] + boneOffset].invBinPose * BoneWeights[0];
	finalBoneVertexTransform 	 += bonesXForms[BoneIDs[1] + boneOffset].globalInv * bonesXForms[BoneIDs[1] + boneOffset].transform * bonesXForms[BoneIDs[1] + boneOffset].invBinPose * BoneWeights[1];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[2] + boneOffset].globalInv * bonesXForms[BoneIDs[2] + boneOffset].transform * bonesXForms[BoneIDs[2] + boneOffset].invBinPose * BoneWeights[2];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[3] + boneOffset].globalInv * bonesXForms[BoneIDs[3] + boneOffset].transform * bonesXForms[BoneIDs[3] + boneOffset].invBinPose * BoneWeights[3]; 

	mat4 vertexTransform = models[DrawID] * finalBoneVertexTransform;
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0);

	vs_out.normal = mat3(transpose(inverse(vertexTransform))) * aNormal;
	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	vs_out.fragPos = modelPos.xyz;

	gl_Position = projection * view * modelPos;

}