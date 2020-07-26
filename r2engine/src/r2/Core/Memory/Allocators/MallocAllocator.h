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
        MallocAllocator();
        explicit MallocAllocator(const utils::MemBoundary& boundary);
        
        ~MallocAllocator();
        
        void* Allocate(u64 size, u64 alignment, u64 offset);
        void Free(void* ptr);
        void Reset() {};
        u32 GetAllocationSize(void* memoryPtr) const;
        
        inline u64 GetTotalBytesAllocated() const {return mAllocatedMemory;}
        inline u64 GetTotalMemory() const {return std::numeric_limits<u32>::max();}
        inline const void* StartPtr() const {return mBoundary.location;}
        static u32 HeaderSize() {return sizeof(utils::Header);}
        inline u64 UnallocatedBytes() const {return GetTotalMemory() - GetTotalBytesAllocated();}
    private:
        utils::MemBoundary mBoundary;
        u64 mAllocatedMemory;
    };
    
#if defined(R2_DEBUG) || defined(R2_RELEASE)
    typedef MemoryArena<MallocAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> MallocArena;
#else
    typedef MemoryArena<MallocAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> MallocArena;
#endif
}

#endif /* MallocAllocator_h */
