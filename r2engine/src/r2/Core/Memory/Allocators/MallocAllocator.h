//
//  MallocAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-06-25.
//

#ifndef MallocAllocator_h
#define MallocAllocator_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Core/Memory/MemoryTagging.h"
#include "r2/Core/Memory/MemoryTracking.h"

namespace r2::mem
{
    class MallocAllocator
    {
    public:
        explicit MallocAllocator(const utils::MemBoundary& boundary);
        
        ~MallocAllocator();
        
        void* Allocate(u64 size, u64 alignment, u64 offset);
        void Free(void* ptr);
        
        u32 GetAllocationSize(void* memoryPtr) const;
        
        inline u64 GetTotalBytesAllocated() const {return 0;}
        inline u64 GetTotalMemory() const {return mBoundary.size;}
        inline const void* StartPtr() const {return nullptr;}
        static u32 HeaderSize() {return sizeof(utils::Header);}
        inline u64 UnallocatedBytes() const {return mBoundary.size;}
    private:
        utils::MemBoundary mBoundary;
    };
    
    typedef MemoryArena<MallocAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> MallocArena;
}

#endif /* MallocAllocator_h */
