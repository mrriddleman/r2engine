#version 450 core

#extension GL_NV_gpu_shader5 : enable

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec4 BoneWeights;
layout (location = 5) in ivec4 BoneIDs;
layout (location = 6) in uint DrawID;

#include "Input/UniformBuffers/Matrices.glsl"
#include "Common/ModelFunctions.glsl"

out VS_OUT
{
	vec3 texCoords; 
	flat uint drawID;
} vs_out;

invariant gl_Position;

void main()
{
	mat4 vertexTransform = GetAnimatedModel(DrawID);

	mat3 normalMatrix = GetNormalMatrix(vertexTransform);

	vec4 worldNormal = vec4(normalMatrix * aNormal, 0);

	vs_out.texCoords = aTexCoord;
	vs_out.drawID = DrawID;
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0) + (worldNormal * 0.05);
	
	gl_Position = projection * view * modelPos;
}