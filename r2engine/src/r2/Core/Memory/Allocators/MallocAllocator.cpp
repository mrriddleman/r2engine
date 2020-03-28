//
//  MallocAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-25.
//
#include "r2pch.h"
#include "MallocAllocator.h"

namespace r2::mem
{
    
    MallocAllocator::MallocAllocator():mAllocatedMemory(0)
    {
        
    }
    
    MallocAllocator::MallocAllocator(const utils::MemBoundary& boundary)
        :mBoundary(boundary)
        , mAllocatedMemory(0)
    {
        
    }
    
    MallocAllocator::~MallocAllocator()
    {
        
    }
    
    void* MallocAllocator::Allocate(u64 size, u64 alignment, u64 offset)
    {
        u64 realSize = size + static_cast<u64>(HeaderSize());
        
        void* pointer = malloc(realSize);
        
        utils::Header* header = (utils::Header*)pointer;
        
        header->size = static_cast<u32>(size);
        
        mAllocatedMemory += realSize;
        
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
