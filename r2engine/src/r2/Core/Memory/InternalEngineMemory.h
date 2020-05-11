//
//  InternalEngineMemory.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//

#ifndef InternalEngineMemory_h
#define InternalEngineMemory_h

#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
namespace r2::mem
{
    struct InternalEngineMemory
    {
        //@TODO(Serge): move the handles out of here
        r2::mem::MemoryArea::Handle internalEngineMemoryHandle = MemoryArea::Invalid;
        r2::mem::MemoryArea::SubArea::Handle permanentStorageHandle = MemoryArea::SubArea::Invalid;
        LinearArena* permanentStorageArena = nullptr;
        StackArena* singleFrameArena = nullptr;
    };
}

#endif /* InternalEngineMemory_h */
