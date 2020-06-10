//
//  Mesh.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-11.
//
#include "r2pch.h"
#include "Mesh.h"

namespace r2::draw
{
    u64 Mesh::MemorySize(u64 numVertices, u64 numIndices, u64 numMaterials, u64 alignment, u64 headerSize, u64 boundsChecking)
    {
        return
            r2::mem::utils::GetMaxMemoryForAllocation( r2::SArray<r2::draw::Vertex>::MemorySize(numVertices), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation( r2::SArray<u32>::MemorySize(numIndices), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation( r2::SArray<r2::draw::MaterialHandle>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);
    }
}
