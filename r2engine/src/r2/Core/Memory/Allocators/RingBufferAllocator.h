//
//  RingBufferAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-26.
//

#ifndef RingBufferAllocator_h
#define RingBufferAllocator_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Core/Memory/MemoryTagging.h"
#include "r2/Core/Memory/MemoryTracking.h"

namespace r2
{
    namespace mem
    {
        class RingBufferAllocator
        {
        public:
            explicit RingBufferAllocator(const utils::MemBoundary& boundary);
            
            ~RingBufferAllocator();
            
            void* Allocate(u32 size, u32 alignment, u32 offset);
            void Free(void* ptr);
            
            bool InUse(void* noptrP) const;
            
            u32 GetAllocationSize(void* memoryPtr) const;
            
            inline u32 GetTotalBytesAllocated() const; 
            inline u32 GetTotalMemory() const {return static_cast<u32>(utils::PointerOffset(mStart, mEnd));}
            inline const void* StartPtr() const {return mStart;}
            inline u32 HeaderSize() const {return sizeof(utils::Header);}
            inline u32 UnallocatedBytes() const {return GetTotalMemory() - GetTotalBytesAllocated();}
            
        private:
            static const u32 FREE_BLOCK = 0x80000000u;
            static const u32 ALLOCATED_BLOCK = 0x7fffffffu;
            
            byte* mStart;
            byte* mEnd;
            byte* mWhereToAllocate;
            byte* mWhereToFree;
        };
    }
}

#endif /* RingBufferAllocator_h */
