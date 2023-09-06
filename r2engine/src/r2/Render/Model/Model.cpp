#include "r2pch.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Animation/Animation.h"

namespace r2::draw
{

	//@TODO(Serge): I think we're allocating too much memory for these. Since our asset system works as 1 big allocation per asset
	// we don't need to be adding the header size, bounds checking or alignment calculations. We should probably remove the
	// r2::mem::utils::GetMaxMemoryForAllocation() calls for everything except the 
	// r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimModel)) / r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model) call


	u64 Skeleton::MemorySizeNoData(u64 numJoints, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(numJoints), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::math::Transform>::MemorySize(numJoints), alignment, headerSize, boundsChecking) + //rest pose
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<u64>::MemorySize(numJoints), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::math::Transform>::MemorySize(numJoints), alignment, headerSize, boundsChecking) + // bind pose
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<s32>::MemorySize(numJoints), alignment, headerSize, boundsChecking); //real parents
	}

	//u64 AnimModel::MemorySizeNoData(u64 boneMapping, u64 boneDataSize, u64 boneInfoSize, u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking)
	//{
	//	u64 totalSize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimModel), alignment, headerSize, boundsChecking) +
	//		r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BoneData>::MemorySize(boneDataSize), alignment, headerSize, boundsChecking) +
	//		r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BoneInfo>::MemorySize(boneInfoSize), alignment, headerSize, boundsChecking) +
	//		r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<const Mesh*>::MemorySize(numMeshes), alignment, headerSize, boundsChecking) +
	//		r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::mat::MaterialName>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);

	//	if (boneMapping > 0)
	//	{
	//		totalSize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<u32>::MemorySize(boneMapping), alignment, headerSize, boundsChecking);
	//	}

	//	return totalSize;
	//		//r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Animation>::MemorySize(numAnimations), alignment, headerSize, boundsChecking);
	//}

	//u64 AnimModel::MemorySizeNoData(u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking)
	//{
	//	return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(AnimModel), alignment, headerSize, boundsChecking) +
	//		Model::ModelMemorySize(numMeshes, numMaterials, alignment, headerSize, boundsChecking) - r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model), alignment, headerSize, boundsChecking);
	//		//r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Mesh>::MemorySize(numMeshes), alignment, headerSize, boundsChecking);
	//
	//}


	u64 Model::MemorySize(u32 numMeshes, u32 numMaterials, u32 numJoints, u32 boneDataSize, u32 boneInfoSize, u32 numAnimations, u32 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 memorySize = r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<const Mesh*>::MemorySize(numMeshes), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::mat::MaterialName>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);

		if ( boneDataSize > 0 && boneInfoSize > 0)
		{
			memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BoneData>::MemorySize(boneDataSize), alignment, headerSize, boundsChecking) +
				r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BoneInfo>::MemorySize(boneInfoSize), alignment, headerSize, boundsChecking);
				
			memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SHashMap<s32>::MemorySize(std::round(boneInfoSize * r2::SHashMap<s32>::LoadFactorMultiplier())), alignment, headerSize, boundsChecking);
		}

		if (numJoints > 0)
		{
			memorySize += Skeleton::MemorySizeNoData(numJoints, alignment, headerSize, boundsChecking);
		}
		

		if (numAnimations > 0)
		{
			memorySize += r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<Animation*>::MemorySize(numAnimations), alignment, headerSize, boundsChecking);
		}

		return memorySize;
	}

    //{
    //    //@NOTE: the passed in u64 numVertices, u64 numIndices, u64 numTextures should be max values
    //    return ModelMemorySize(numMeshes, numMaterials, alignment, headerSize, boundsChecking) +
    //        numMeshes * Mesh::MemorySize(numVertices, numIndices, alignment, headerSize, boundsChecking);
    //}

    u64 Model::ModelMemorySize(u64 numMeshes, u64 numMaterials, u64 alignment, u32 headerSize, u32 boundsChecking)
    {
        return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(Model), alignment, headerSize, boundsChecking) +
            r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<const Mesh*>::MemorySize(numMeshes), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::mat::MaterialName>::MemorySize(numMaterials), alignment, headerSize, boundsChecking);
    }
}