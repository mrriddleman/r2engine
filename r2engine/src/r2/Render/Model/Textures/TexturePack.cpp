#include "r2pch.h"
#include "r2/Render/Model/Textures/TexturePack.h"
#include "r2/Utils/Utils.h"

namespace r2::draw::tex
{
	const u32 NUM_ARRAYS = 12; //number of arrays we have in the texture pack
	u64 TexturePackMemorySize(u64 capacity, u64 alignment, u32 headerSize, u32 boundsChecking)
	{
		return r2::mem::utils::GetMaxMemoryForAllocation(sizeof(TexturePack), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<r2::asset::Asset>::MemorySize(capacity), alignment, headerSize, boundsChecking) * NUM_ARRAYS;
	}
}