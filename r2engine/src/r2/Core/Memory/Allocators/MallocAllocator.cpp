//
//  MallocAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-25.
//

#include "MallocAllocator.h"

namespace r2::mem
{
    MallocAllocator::MallocAllocator(const utils::MemBoundary& boundary)
        :mBoundary(boundary)
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
        
        return utils::PointerAdd(pointer, HeaderSize());
    }
    
    void MallocAllocator::Free(void* ptr)
    {
        void* realPtr = utils::PointerSubtract(ptr, HeaderSize());
        free(realPtr);
    }
    
    u32 MallocAllocator::GetAllocationSize(void* memoryPtr) const
    {
        utils::Header* header = (utils::Header*)utils::PointerSubtract(memoryPtr, HeaderSize());
        return static_cast<u32>(header->size);
    }
}
