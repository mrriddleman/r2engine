//
//  PoolAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//

#ifndef PoolAllocator_h
#define PoolAllocator_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Core/Memory/MemoryTagging.h"
#include "r2/Core/Memory/MemoryTracking.h"

namespace r2
{
    namespace mem
    {
        class PoolAllocator
        {
        public:
            explicit PoolAllocator(const utils::MemBoundary& boundary);
            ~PoolAllocator();
            
            void* Allocate(u64 size, u64 alignment, u64 offset);
            void Free(void* ptr);
            
            u32 GetAllocationSize(void* memoryPtr) const;
            u64 TotalElements() const {return mFreeList.NumElements();}
            u64 NumElementsAllocated() const {return mNumAllocations;}
            //I think this is wrong since this doesn't take into account the alignment
            inline u64 GetTotalBytesAllocated() const {return mElementSize * mNumAllocations;}
            inline u64 GetTotalMemory() const {return utils::PointerOffset(mStart, mEnd);}
            inline const void* StartPtr() const {return mStart;}
        private:
            class Freelist
            {
            public:
                Freelist(void* start, void* end, u64 elementSize, u64 alignment, u64 offset);
                inline void* Obtain(void);
                inline void Return(void* memoryPtr);
                inline u64 NumElements() const {return mNumElements;}
            private:
                Freelist* mNext;
                u64 mNumElements;
            };
            
            Freelist mFreeList;
            
            void* mStart;
            void* mEnd;
            const u64 mElementSize;
            const u64 mAlignment;
            u64 mNumAllocations;
        };
        
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        typedef MemoryArena<PoolAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> PoolArena;
#else
        typedef MemoryArena<PoolAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> PoolArena;
#endif
    }
}

#endif /* PoolAllocator_h */
