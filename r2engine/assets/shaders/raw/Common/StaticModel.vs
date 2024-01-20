#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/UniformBuffers/Matrices.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Common/CommonFunctions.glsl"

invariant gl_Position;

out VS_OUT
{
    flat uint drawID;
    flat uint materialIndex;
    vec3 texCoords; 
} vs_out;

void main()
{
    vec4 modelPos = models[DrawID] * vec4(aPos, 1.0);
	gl_Position = projection * view * modelPos;

    vs_out.materialIndex = GetLocalMeshOrMaterialIndex(aTexCoord);
    vs_out.drawID = DrawID;
    vs_out.texCoords= aTexCoord;
}