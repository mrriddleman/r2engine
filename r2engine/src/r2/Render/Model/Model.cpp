#include "r2pch.h"
#include "Model.h"

namespace r2::draw
{
    u64 Model::MemorySize(u64 numMeshes, u64 numVertices, u64 numIndices, u64 numTextures)
    {
        //@NOTE: the passed in u64 numVertices, u64 numIndices, u64 numTextures should be max values
        return sizeof(Model) +
            r2::SArray<Mesh>::MemorySize(numMeshes) +
            numMeshes * (Mesh::MemorySize(numVertices, numIndices, numTextures));
    }
}