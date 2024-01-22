#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Input/VertexLayouts/StaticVertexInput.glsl"
#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Common/CommonFunctions.glsl"
#include "Common/ModelFunctions.glsl"

void main()
{
	gl_Position = GetStaticModel(DrawID, GetLocalMeshOrMaterialIndex(aTexCoord1)) * vec4(aPos, 1.0);
}