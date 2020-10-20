#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec4 BoneWeights;
layout (location = 4) in ivec4 BoneIDs;
layout (location = 5) in uint DrawID;

layout (std140, binding = 0) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

layout (std140, binding = 0) buffer Models
{
	mat4 models[];
};

layout (std430, binding = 2) buffer BoneTransforms
{
	mat4 bonesXForms[];
};

layout (std140, binding = 3) buffer BoneTransformOffsets
{
	ivec4 boneOffsets[];
};

out VS_OUT
{
	vec3 normal;
	vec3 texCoords;
	flat uint drawID;
} vs_out;

void main()
{
	int boneOffset = boneOffsets[DrawID].x;
	mat4 finalBoneVertexTransform = bonesXForms[BoneIDs[0] + boneOffset] * BoneWeights[0];
	finalBoneVertexTransform 	 += bonesXForms[BoneIDs[1] + boneOffset] * BoneWeights[1];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[2] + boneOffset] * BoneWeights[2];
	finalBoneVertexTransform	 += bonesXForms[BoneIDs[3] + boneOffset] * BoneWeights[3]; 

	mat4 vertexTransform = models[DrawID] * finalBoneVertexTransform;
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0);

	vs_out.normal = mat3(transpose(inverse(vertexTransform))) * aNormal;
	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	gl_Position = projection * view * modelPos;

}