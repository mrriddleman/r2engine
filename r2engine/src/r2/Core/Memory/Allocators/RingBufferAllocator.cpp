//
//  RingBufferAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-26.
//

#include "RingBufferAllocator.h"

namespace r2
{
    namespace mem
    {
        RingBufferAllocator::RingBufferAllocator(const utils::MemBoundary& boundary)
        : mStart((byte*)boundary.location)
        , mEnd((byte*)utils::PointerAdd(boundary.location, boundary.size))
        , mWhereToAllocate(mStart)
        , mWhereToFree(mStart)
        {
            
        }
        
        RingBufferAllocator::~RingBufferAllocator()
        {
            R2_CHECK(mWhereToFree == mWhereToAllocate, "We shouldn't have anymore memory allocated in us!");
            mStart = nullptr;
            mEnd = nullptr;
            mWhereToAllocate = nullptr;
            mWhereToFree = nullptr;
        }
        
        void* RingBufferAllocator::Allocate(u32 size, u32 alignment, u32 offset)
        {
            if ((size + sizeof(utils::Header) + (alignment-1)) > GetTotalMemory())
            {
                R2_CHECK(false, "Trying to allocate more memory than we have!");
                return nullptr;
            }
            
            R2_CHECK(alignment % 4 == 0, "Alignment should be multiples of 4!");
            
            size = ((size + 3)/4)*4;
            
            byte* pointer = (byte*)utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(mWhereToAllocate, offset + sizeof(utils::Header)), alignment), offset + sizeof(utils::Header));
            
            utils::Header* ptrToHeader = (utils::Header*)pointer;
            byte* ptrData = (byte*)utils::PointerAdd(ptrToHeader, sizeof(utils::Header));
            
            pointer = (byte*)utils::PointerAdd(pointer, size + sizeof(utils::Header));
            
            if (pointer >= mEnd)
            {
                ptrToHeader->size = static_cast<u32>(utils::PointerOffset(ptrToHeader, mEnd)) | FREE_BLOCK;
                
                pointer = (byte*)utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(mStart, offset + sizeof(utils::Header)), alignment), offset + sizeof(utils::Header));
                ptrToHeader = (utils::Header*)pointer;
                ptrData = (byte*)utils::PointerAdd(ptrToHeader, sizeof(utils::Header));
                pointer = (byte*)utils::PointerAdd(ptrData, size);
            }
            
            R2_CHECK(!InUse(pointer), "We've ran out of memory!");

            ptrToHeader->size = size;
            
            if (pointer == mEnd)
            {
                pointer = mStart;
            }
            
            mWhereToAllocate = pointer;
            return ptrData;
        }
        
        void RingBufferAllocator::Free(void* ptr)
        {
            R2_CHECK(ptr != nullptr, "Can't free nullptr bro!");
            
            R2_CHECK(InUse(ptr), "The ptr should be in use!");
            
            utils::Header* header = (utils::Header*)utils::PointerSubtract(ptr, sizeof(utils::Header));
            
            R2_CHECK((header->size & FREE_BLOCK) == 0, "Make sure that it hasn't been freed!");
            
            while (mWhereToFree != mWhereToAllocate)
            {
                utils::Header* header = (utils::Header*)mWhereToFree;
                
                if ((header->size & FREE_BLOCK) == 0)
                {
                    break;
                }
                
                mWhereToFree += header->size & ALLOCATED_BLOCK;
                
                if (mWhereToFree == mEnd)
                {
                    mWhereToFree = mStart;
                }
            }
        }
        
        bool RingBufferAllocator::InUse(void* noptrP) const
        {
            if (mWhereToFree == mWhereToAllocate)
            {
                return false;
            }
            
            if (mWhereToAllocate > mWhereToFree)
            {
                return noptrP >= mWhereToFree && noptrP < mWhereToAllocate;
            }
            
            return noptrP >= mWhereToFree || noptrP < mWhereToAllocate;
        }
        
        inline u32 RingBufferAllocator::GetTotalBytesAllocated() const
        {
            if (mWhereToAllocate > mWhereToFree)
            {
                return static_cast<u32>(utils::PointerOffset(mWhereToFree, mWhereToAllocate));
            }
            
            return static_cast<u32>(utils::PointerOffset(mWhereToAllocate, mWhereToFree));
        }
        
        u32 RingBufferAllocator::GetAllocationSize(void* memoryPtr) const
        {
            R2_CHECK(memoryPtr != nullptr, "Why you giving me a nullptr bro?");
            
            R2_CHECK(InUse(memoryPtr), "This memory should be in Use!");
            
            utils::Header* header = (utils::Header*)utils::PointerSubtract(memoryPtr, sizeof(utils::Header));
            return header->size;
        }
    }
}
