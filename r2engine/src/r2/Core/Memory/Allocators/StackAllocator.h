//
//  StackAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//

#ifndef StackAllocator_h
#define StackAllocator_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Core/Memory/MemoryTagging.h"
#include "r2/Core/Memory/MemoryTracking.h"

#ifndef STACK_ALLOCATOR_CHECK_FREE_LIFO_ORDER
    #define STACK_ALLOCATOR_CHECK_FREE_LIFO_ORDER 1
#endif

namespace r2
{
    namespace mem
    {
        class StackAllocator
        {
        public:
            explicit StackAllocator(const utils::MemBoundary& boundary);
            
            ~StackAllocator();
            
            void* Allocate(u64 size, u64 alignment, u64 offset);
            void Free(void* ptr);
            
            u32 GetAllocationSize(void* memoryPtr) const;
            
            inline u64 GetTotalBytesAllocated() const {return utils::PointerOffset(mStart, mCurrent);}
            inline u64 GetTotalMemory() const {return utils::PointerOffset(mStart, mEnd);}
            inline const void* StartPtr() const {return mStart;}
            inline u32 HeaderSize() const;
            inline u64 UnallocatedBytes() const {return utils::PointerOffset(mCurrent, mEnd);}
        private:
            byte* mStart;
            byte* mEnd;
            byte* mCurrent;
            
            s32 mLastAllocationID;
        };
        
        //@TODO(Serge): Add in more verbose tracking etc
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        typedef MemoryArena<StackAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> StackArena;
#else
        typedef MemoryArena<StackAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> StackArena;
#endif
    
    }
}

#endif /* StackAllocator_h */
