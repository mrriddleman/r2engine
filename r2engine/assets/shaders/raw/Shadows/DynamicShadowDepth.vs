#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec4 BoneWeights;
layout (location = 5) in ivec4 BoneIDs;
layout (location = 6) in uint DrawID;

#include "Common/ModelFunctions.glsl"

void main()
{
	mat4 vertexTransform = GetAnimatedModel(DrawID);
	gl_Position = vertexTransform * vec4(aPos, 1.0);
}