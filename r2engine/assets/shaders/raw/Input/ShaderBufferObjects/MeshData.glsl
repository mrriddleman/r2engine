#ifndef GLSL_MESH_DATA
#define GLSL_MESH_DATA

#include "Input/ShaderBufferObjects/MaterialData.glsl"

struct MeshData
{
	mat4 globalTransform;
	mat4 globalInvTransform;
};

layout (std430, binding=10) buffer MeshDataBuffer
{
	MeshData meshData[];
};

uint GetMeshIndex(uint drawID, uint localMeshIndex)
{
	highp uint meshIndex = localMeshIndex + materialOffsets[drawID].w;
	return meshIndex;
}

MeshData GetMeshData(uint drawID, uint localMeshIndex)
{
	highp uint meshIndex = GetMeshIndex(drawID, localMeshIndex);
	return meshData[meshIndex];
}

#endif