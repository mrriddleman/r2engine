#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aBoneWeights;
layout (location = 4) in ivec4 aBoneIDs;

const int NUM_BONE_TRANSFORMATIONS = 100;
const int MAX_BONE_WEIGHTS = 4;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneTransformations[NUM_BONE_TRANSFORMATIONS];

void main()
{
	mat4 finalBoneVertexTransform 	 = boneTransformations[aBoneIDs[0]] * aBoneWeights[0];
	finalBoneVertexTransform 		+= boneTransformations[aBoneIDs[1]] * aBoneWeights[1];
	finalBoneVertexTransform 		+= boneTransformations[aBoneIDs[2]] * aBoneWeights[2];
	finalBoneVertexTransform 		+= boneTransformations[aBoneIDs[3]] * aBoneWeights[3];

	mat4 vertexTransform = model * finalBoneVertexTransform;
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0);

    gl_Position = projection * view * modelPos;
}