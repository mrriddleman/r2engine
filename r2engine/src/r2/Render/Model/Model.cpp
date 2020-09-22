#include "r2pch.h"
#include "Model.h"


namespace r2::draw
{

	//@TODO(Serge): I think we're allocating too much memory for these. Since our asset system works as 1 big allocation per asset
	// we don't need to be adding the header size, bounds checking or alignment calculations. We should probably remove the
	// r2::mem::utils::GetMaxMemoryForAllocation() calls for everything except the 
	// r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimModel)) / r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model) call

	u64 Skeleton::MemorySizeNoData(u64 numChildren, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Skeleton>::MemorySize(numChildren), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Skeleton), alignment, headerSize, boundsChecking);
	}
	


	u64 AnimModel::MemorySizeNoData(u64 boneMapping, u64 boneDataSize, u64 boneInfoSize, u64 numMeshes, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimModel), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<u32>::MemorySize(boneMapping), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BoneData>::MemorySize(boneDataSize), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BoneInfo>::MemorySize(boneInfoSize), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Mesh>::MemorySize(numMeshes), alignment, headerSize, boundsChecking);
			//r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Animation>::MemorySize(numAnimations), alignment, headerSize, boundsChecking);
	}

    u64 Model::MemorySize(u64 numMeshes, u64 numVertices, u64 numIndices, u64 numTextures, u64 headerSize, u64 boundsChecking, u64 alignment)
    {
        //@NOTE: the passed in u64 numVertices, u64 numIndices, u64 numTextures should be max values
        return ModelMemorySize(numMeshes, alignment, headerSize, boundsChecking) +
            numMeshes * Mesh::MemorySize(numVertices, numIndices, numTextures, alignment, headerSize, boundsChecking);
    }

    u64 Model::ModelMemorySize(u64 numMeshes, u64 alignment, u32 headerSize, u32 boundsChecking)
    {
        return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Mesh>::MemorySize(numMeshes), alignment, headerSize, boundsChecking);
    }
}