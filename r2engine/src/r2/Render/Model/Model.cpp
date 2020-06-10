#include "r2pch.h"
#include "Model.h"


namespace r2::draw
{
    u64 Model::MemorySize(u64 numMeshes, u64 numVertices, u64 numIndices, u64 numTextures, u64 headerSize, u64 boundsChecking, u64 alignment)
    {
        
        //@NOTE: the passed in u64 numVertices, u64 numIndices, u64 numTextures should be max values
        return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Mesh>::MemorySize(numMeshes), alignment, headerSize, boundsChecking) +
            numMeshes * Mesh::MemorySize(numVertices, numIndices, numTextures, alignment, headerSize, boundsChecking);
    }
}