//
//  MallocAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-25.
//
#include "r2pch.h"
#include "MallocAllocator.h"
#include <algorithm>
namespace r2::mem
{
    
    MallocAllocator::MallocAllocator():mAllocatedMemory(0), mHighWaterMark(0)
    {
        
    }
    
    MallocAllocator::MallocAllocator(const utils::MemBoundary& boundary)
        :mBoundary(boundary)
        , mAllocatedMemory(0)
        , mHighWaterMark(0)
    {
        
    }
    
    MallocAllocator::~MallocAllocator()
    {
        printf("Malloc allocator High water mark is: %llu\n", mHighWaterMark);
    }
    
    void* MallocAllocator::Allocate(u64 size, u64 alignment, u64 offset)
    {
        u64 realSize = size + static_cast<u64>(HeaderSize());
        
        void* pointer = malloc(realSize);
        
        utils::Header* header = (utils::Header*)pointer;
        
        header->size = static_cast<u32>(size);
        
        mAllocatedMemory += realSize;

        mHighWaterMark = std::max(mHighWaterMark, mAllocatedMemory);
        
        return utils::PointerAdd(pointer, HeaderSize());
    }
    
    void MallocAllocator::Free(void* ptr)
    {
        u64 allocationSize = GetAllocationSize(ptr);
        void* realPtr = utils::PointerSubtract(ptr, HeaderSize());
        free(realPtr);
        mAllocatedMemory -= HeaderSize() + allocationSize;
    }
    
    u32 MallocAllocator::GetAllocationSize(void* memoryPtr) const
    {
        utils::Header* header = (utils::Header*)utils::PointerSubtract(memoryPtr, HeaderSize());
        return static_cast<u32>(header->size);
    }

}

namespace r2::mem::utils
{
    MallocArena* EmplaceMallocArenaInMemoryBoundary(const MemBoundary& boundary, const char* file, s32 line, const char* description)
    {
		//we need to figure out how much space we have and calculate a memory boundary for the Allocator
		R2_CHECK(boundary.size > sizeof(MallocArena), "subArea size(%llu) must be greater than sizeof(MallocArena)(%lu)!", boundary.size, sizeof(MallocArena));
		if (boundary.size <= sizeof(MallocArena))
		{
			return nullptr;
		}

		MemBoundary mallocAllocatorBoundary;
        mallocAllocatorBoundary.location = PointerAdd(boundary.location, sizeof(MallocArena));
        mallocAllocatorBoundary.size = boundary.size - sizeof(MallocArena);

        MallocArena* mallocArena = new (boundary.location) MallocArena(mallocAllocatorBoundary);

		return mallocArena;
    }
}