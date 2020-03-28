//
//  PoolAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//
#include "r2pch.h"
#include "PoolAllocator.h"

namespace r2
{
    namespace mem
    {

        
        PoolAllocator::PoolAllocator(const utils::MemBoundary& boundary):
        mFreeList(boundary.location, utils::PointerAdd(boundary.location, boundary.size), boundary.elementSize, boundary.alignment, boundary.offset),
        mStart((byte*)boundary.location),
        mEnd((byte*)utils::PointerAdd(boundary.location, boundary.size)),
        mElementSize(boundary.elementSize),
        mAlignment(boundary.alignment),
        mNumAllocations(0)
        {
            
        }
        
        PoolAllocator::~PoolAllocator()
        {
#if R2_CHECK_ALLOCATIONS_ON_DESTRUCTION
            R2_CHECK(mNumAllocations == 0, "We shouldn't have any allocations left in the pool!");
#endif
        }
        
        void* PoolAllocator::Allocate(u64 size, u64 alignment, u64 offset)
        {
            R2_CHECK(size <= mElementSize, "Size is greater than the element size");
            R2_CHECK(alignment <= mAlignment, "alignment is greater than the max alignment");
            
            if (mNumAllocations + 1 > mFreeList.NumElements())
            {
                return nullptr;
            }

            void* pointer = mFreeList.Obtain();
            
            if (pointer != nullptr)
            {
                ++mNumAllocations;
            }
            
            return pointer;
        }
        
        void PoolAllocator::Free(void* ptr)
        {
            R2_CHECK(ptr != nullptr, "Why are you passing a nullptr to free?");
            R2_CHECK(ptr >= mStart && ptr < mEnd, "Pointer should be within the pool!");
            mFreeList.Return(ptr);
            --mNumAllocations;
        }
        
        u32 PoolAllocator::GetAllocationSize(void* memoryPtr) const
        {
            return static_cast<u32>(mElementSize);
        }
        
        PoolAllocator::Freelist::Freelist(void* start, void* end, u64 elementSize, u64 alignment, u64 offset)
        {
            R2_CHECK(elementSize >= sizeof(Freelist), "elementSize must be greater than or equal to a freelist object which is %zu bytes", sizeof(Freelist));
            void* pointer = utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(start, offset), alignment), offset);
            
            union
            {
                void* as_void;
                byte* as_byte;
                Freelist* as_self;
            };
            
            as_void = utils::PointerAdd(pointer, offset);
            
            u64 totalMem = utils::PointerOffset(as_void, end);
            u64 adjustment = totalMem % elementSize;
            
            as_void = utils::PointerAdd(as_void, adjustment);
            
            totalMem = utils::PointerOffset(as_void, end);
            R2_CHECK(totalMem % elementSize == 0, "totalMem should be a multiple of elementSize");
            mNumElements = totalMem/elementSize;
            
            mNext = as_self;
            as_byte = (byte*)utils::PointerAdd(as_byte, elementSize);
        
            Freelist* runner = mNext;
            for (u64 i = 1; i < mNumElements; ++i)
            {
                runner->mNext = as_self;
                runner = as_self;
                as_byte = (byte*)utils::PointerAdd(as_byte, elementSize);
            }
        }
        
        inline void* PoolAllocator::Freelist::Obtain(void)
        {
            if (mNext == nullptr)
            {
                return nullptr;
            }
            
            Freelist* head = mNext;
            mNext = head->mNext;
            return head;
        }
        
        inline void PoolAllocator::Freelist::Return(void* memoryPtr)
        {
            Freelist* head = static_cast<Freelist*>(memoryPtr);
            head->mNext = mNext;
            mNext = head;
        }
    }
}

namespace r2::mem::utils
{
    PoolArena* EmplacePoolArena(MemoryArea::MemorySubArea& subArea, u64 elementSize, const char* file, s32 line, const char* description)
    {
        //we need to figure out how much space we have and calculate a memory boundary for the Allocator
        R2_CHECK(subArea.mBoundary.size > sizeof(PoolArena), "subArea size(%llu) must be greater than sizeof(PoolArena)(%lu)!", subArea.mBoundary.size, sizeof(PoolArena));
        if (subArea.mBoundary.size <= sizeof(PoolArena))
        {
            return nullptr;
        }
        
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        elementSize = elementSize + BasicBoundsChecking::SIZE_BACK + BasicBoundsChecking::SIZE_FRONT;
#endif
        u64 leftOverSize = subArea.mBoundary.size - sizeof(PoolArena);
        u64 modResult = leftOverSize % elementSize;
        void* poolAllocatorStartPtr = PointerAdd(subArea.mBoundary.location, sizeof(PoolArena) + modResult);
        leftOverSize -= modResult;
        
        MemBoundary poolAllocatorBoundary;
        poolAllocatorBoundary.location = poolAllocatorStartPtr;
        poolAllocatorBoundary.size = leftOverSize;
        poolAllocatorBoundary.elementSize = elementSize;
        poolAllocatorBoundary.alignment = elementSize;
        poolAllocatorBoundary.offset = 0; //?

        PoolArena* stackArena = new (subArea.mBoundary.location) PoolArena(subArea, poolAllocatorBoundary);
        
        return stackArena;
    }
}
