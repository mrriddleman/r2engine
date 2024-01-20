#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/DynamicVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Common/ModelFunctions.glsl"
#include "Common/CommonFunctions.glsl"

out VS_OUT
{
	vec3 texCoords; 
	flat uint materialIndex;
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
	vs_out.materialIndex = GetLocalMeshOrMaterialIndex(aTexCoord);
	vec4 modelPos = vertexTransform * vec4(aPos, 1.0) + (worldNormal * 0.05);
	
	gl_Position = projection * view * modelPos;
}