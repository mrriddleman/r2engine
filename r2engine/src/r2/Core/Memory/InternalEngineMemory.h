//
//  InternalEngineMemory.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//

#ifndef InternalEngineMemory_h
#define InternalEngineMemory_h

#include "r2/Core/Memory/Allocators/LinearAllocator.h"

namespace r2::mem
{
    struct InternalEngineMemory
    {
        //@TODO(Serge): move the handles out of here
        r2::mem::MemoryArea::Handle internalEngineMemoryHandle;
        r2::mem::MemoryArea::MemorySubArea::Handle permanentStorageHandle;
        LinearArena* permanentStorageArena;
    };
}

#endif /* InternalEngineMemory_h */
