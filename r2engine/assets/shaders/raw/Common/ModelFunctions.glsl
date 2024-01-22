#ifndef GLSL_MODEL_FUNCTIONS
#define GLSL_MODEL_FUNCTIONS

#include "Input/ShaderBufferObjects/ModelData.glsl"
#include "Input/ShaderBufferObjects/MeshData.glsl"

mat4 GetStaticModel(uint drawID, uint localMeshIndex)
{
	MeshData meshData = GetMeshData(drawID, localMeshIndex);
	return models[drawID] * meshData.globalTransform; 
}



#endif