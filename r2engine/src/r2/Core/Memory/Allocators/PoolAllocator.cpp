//
//  PoolAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//

#include "PoolAllocator.h"

namespace r2
{
    namespace mem
    {
        PoolAllocator::PoolAllocator(const utils::MemBoundary& boundary):
        mFreeList(boundary.location, utils::PointerAdd(boundary.location, boundary.size), boundary.elementSize, boundary.alignment, boundary.offset),
        mStart(boundary.location),
        mEnd(utils::PointerAdd(boundary.location, boundary.size)),
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
            
            void* pointer = mFreeList.Obtain();
            
            if (pointer != nullptr)
            {
                ++mNumAllocations;
            }
            
            return pointer;
        }
        
        void PoolAllocator::Free(void* ptr)
        {
            mFreeList.Return(ptr);
            --mNumAllocations;
        }
        
        u32 PoolAllocator::GetAllocationSize(void* memoryPtr) const
        {
            return static_cast<u32>(mElementSize);
        }
        
        PoolAllocator::Freelist::Freelist(void* start, void* end, u64 elementSize, u64 alignment, u64 offset)
        {
            R2_CHECK(elementSize >= sizeof(uptr), "elementSize must be greater than or equal to a pointer");
            void* pointer = utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(start, offset), alignment), offset);
            
            union
            {
                void* as_void;
                byte* as_byte;
                Freelist* as_self;
            };
            
            as_self = static_cast<Freelist*>(utils::PointerAdd(pointer, offset));
            
            u64 totalMem = utils::PointerOffset(as_void, end);
            R2_CHECK(totalMem % elementSize == 0, "totalMem should be a multiple of elementSize");
            u64 numElements = totalMem/elementSize;
            
            // assume as_self points to the first entry in the free list
            mNext = as_self;
            as_byte += elementSize;
            
            // initialize the free list - make every m_next of each element point to the next element in the list
            Freelist* runner = mNext;
            for (u64 i=1; i<numElements; ++i)
            {
                runner->mNext = as_self;
                runner = as_self;
                as_byte += elementSize;
            }
            
            runner->mNext = nullptr;
        }
        
        inline void* PoolAllocator::Freelist::Obtain(void)
        {
            if(mNext == nullptr)
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
