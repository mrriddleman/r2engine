//
//  StackAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//

#include "StackAllocator.h"

namespace r2
{
    namespace mem
    {
        
        /*
         Stack Header (Each 4 bytes)
         +-----------------+-------------------+---------------+
         | Allocation Size | Allocation Offset | Allocation ID |
         +-----------------+-------------------+---------------+
         */
        
        namespace
        {
            static const u64 SIZE_OF_ALLOCATION_OFFSET = sizeof(u32);
            static_assert(SIZE_OF_ALLOCATION_OFFSET == 4, "Allocation offset has wrong size.");
            static const u64 NUM_HEADERS = 3;
        }
        
        StackAllocator::StackAllocator(const utils::MemBoundary& boundary):mStart((byte*)boundary.location), mEnd((byte*)utils::PointerAdd(boundary.location, boundary.size)), mCurrent(mStart), mLastAllocationID(-1)
        {
            
        }
        
        StackAllocator::~StackAllocator()
        {
#if R2_CHECK_ALLOCATIONS_ON_DESTRUCTION
            R2_CHECK(mStart == mCurrent, "We still have Memory Allocated!!");
#endif
            mStart = nullptr;
            mEnd = nullptr;
            mCurrent = nullptr;
        }
        
        void* StackAllocator::Allocate(u64 size, u64 alignment, u64 offset)
        {
            u64 originalSize = size;
            u64 originalOffset = offset;
            size += SIZE_OF_ALLOCATION_OFFSET*NUM_HEADERS;
            offset += SIZE_OF_ALLOCATION_OFFSET*NUM_HEADERS;
            
            const u32 allocationOffset = static_cast<u32>(utils::PointerOffset(mStart, mCurrent));
            
            void* pointer = utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(mCurrent, offset), alignment), offset);
            
            if(utils::PointerAdd(pointer, size) > mEnd)
            {
                R2_CHECK(false, "We can't fit that size!");
                return nullptr;
            }
            
            mCurrent = (byte*)utils::PointerAdd(pointer, size);
            
            union
            {
                void * as_void;
                byte * as_byte;
                u32 * as_u32;
            };
            
            as_void = pointer;
            
            *as_u32 = static_cast<u32>(originalSize);
            as_void = utils::PointerAdd(as_void, SIZE_OF_ALLOCATION_OFFSET);
            *as_u32 = allocationOffset;
            as_void = utils::PointerAdd(as_void, SIZE_OF_ALLOCATION_OFFSET);
            *as_u32 = ++mLastAllocationID;
            as_void = utils::PointerAdd(as_void, SIZE_OF_ALLOCATION_OFFSET);
            
            R2_CHECK(utils::IsAligned(utils::PointerAdd(as_void, originalOffset), alignment), "The pointer to the actual memory is not aligned!!!!!");
            
            return as_void;
        }
        
        void StackAllocator::Free(void* memoryPtr)
        {
            R2_CHECK(memoryPtr != nullptr, "Why you giving me a nullptr bro?");
            
            R2_CHECK(utils::PointerSubtract(memoryPtr, SIZE_OF_ALLOCATION_OFFSET*2) >= mStart, "You're outside of the proper allocator range bro!");
            
            R2_CHECK(memoryPtr < mCurrent, "The memoryPtr can't be beyond our current pointer!");

            union
            {
                void * as_void;
                byte * as_byte;
                u32 * as_u32;
            };
            
            as_void = memoryPtr;

            //Should give us the allocation id
            as_void = utils::PointerSubtract(as_void, SIZE_OF_ALLOCATION_OFFSET);
            
#ifdef STACK_ALLOCATOR_CHECK_FREE_LIFO_ORDER
            const u32 allocationId = *as_u32;
            R2_CHECK(allocationId == mLastAllocationID, "Hey you're trying to free memory out of order!");
#endif
            as_void = utils::PointerSubtract(as_void, SIZE_OF_ALLOCATION_OFFSET);
            const u32 allocationOffset = *as_u32;
            
            mCurrent = (byte*)utils::PointerAdd(mStart, allocationOffset);
            
            --mLastAllocationID;
        }
        
        u32 StackAllocator::GetAllocationSize(void* memoryPtr) const
        {
            R2_CHECK(memoryPtr != nullptr, "Why you giving me a nullptr bro?");
            
            R2_CHECK(utils::PointerSubtract(memoryPtr, SIZE_OF_ALLOCATION_OFFSET*NUM_HEADERS) >= mStart, "You're outside of the proper allocator range bro!");
            
            R2_CHECK(memoryPtr < mCurrent, "The memoryPtr can't be beyond our current pointer!");

            union
            {
                void * as_void;
                byte * as_byte;
                u32 * as_u32;
            };
            
            as_void = memoryPtr;
            
            as_void = utils::PointerSubtract(as_void, SIZE_OF_ALLOCATION_OFFSET*(NUM_HEADERS));
            
            const u32 allocationSize = *as_u32;
            
            return allocationSize;
        }
        
        u32 StackAllocator::HeaderSize() const
        {
            return static_cast<u32>(SIZE_OF_ALLOCATION_OFFSET * NUM_HEADERS);
        }
    }
}

namespace r2::mem::utils
{
    StackArena* EmplaceStackArena(MemoryArea::MemorySubArea& subArea, const char* file, s32 line, const char* description)
    {
        //we need to figure out how much space we have and calculate a memory boundary for the Allocator
        R2_CHECK(subArea.mBoundary.size > sizeof(StackArena), "subArea size(%llu) must be greater than sizeof(StackArena)(%lu)!", subArea.mBoundary.size, sizeof(StackArena));
        if (subArea.mBoundary.size <= sizeof(StackArena))
        {
            return nullptr;
        }
        
        MemBoundary stackAllocatorBoundary;
        stackAllocatorBoundary.location = PointerAdd(subArea.mBoundary.location, sizeof(StackArena));
        stackAllocatorBoundary.size = subArea.mBoundary.size - sizeof(StackArena);
        
        StackArena* stackArena = new (subArea.mBoundary.location) StackArena(subArea, stackAllocatorBoundary);
        
        return stackArena;
    }
}
