//
//  LinearAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-11.
//

#include "LinearAllocator.h"

namespace r2
{
    namespace mem
    {
        LinearAllocator::LinearAllocator(const utils::MemBoundary& boundary):mStart(boundary.location), mEnd(utils::PointerAdd(mStart, boundary.size)), mCurrent(mStart)
        {
            
        }
        
        void* LinearAllocator::Allocate(u64 size, u64 alignment, u64 offset)
        {
           //this should point to before the offset and header
            void* pointer = utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(mCurrent, offset + sizeof(utils::Header)), alignment), offset + sizeof(utils::Header));
            
            if(static_cast<byte*>(utils::PointerAdd(pointer, size + sizeof(utils::Header))) >= static_cast<byte*>(mEnd))
            {
                R2_CHECK(false, "We can't fit that size!");
                return nullptr;
            }
            
            mCurrent = utils::PointerAdd(pointer, size + sizeof(utils::Header));
            
            utils::Header* header = (utils::Header*)pointer;
            header->size = size;
            
            R2_CHECK(utils::IsAligned(utils::PointerAdd(pointer, sizeof(utils::Header) + offset), alignment), "The pointer to the actual memory is not aligned!!!!!");
        
            return utils::PointerAdd(pointer, sizeof(utils::Header));
        }
        
        void LinearAllocator::Free(void* ptr)
        {
            //Doesn't do anything - don't want fragmentation my dude!
        }
        
        void LinearAllocator::Reset(void)
        {
            mCurrent = mStart;
        }
        
        u32 LinearAllocator::GetAllocationSize(void* memoryPtr) const
        {
            R2_CHECK(memoryPtr != nullptr, "Why you giving me a nullptr bro?");
            R2_CHECK(utils::PointerSubtract(memoryPtr, sizeof(utils::Header)) < mStart, "You're outside of the proper allocator range bro!");
            
            utils::Header* header = (utils::Header*)utils::PointerSubtract(memoryPtr, sizeof(utils::Header));

            return header->size;
        }
        
        u64 LinearAllocator::GetTotalBytesAllocated() const
        {
            return utils::PointerOffset(mEnd, mStart);
        }
    }
}
