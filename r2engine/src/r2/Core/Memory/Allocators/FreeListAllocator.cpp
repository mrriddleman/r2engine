//
//  FreeListAllocator.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-10-15.
//
#include "r2pch.h"
#include "FreeListAllocator.h"
//https://github.com/mtrebi/memory-allocators/blob/master/src/FreeListAllocator.cpp
namespace r2::mem
{
    FreeListAllocator::FreeListAllocator(const utils::MemBoundary& boundary)
    : mStart((byte*)boundary.location)
    , mTotalSize(boundary.size)
    , mUsed(0)
    , mPeak(0)
    , mPolicy(boundary.policy)
    {
        Reset();
    }
    
    FreeListAllocator::~FreeListAllocator()
    {
#if R2_CHECK_ALLOCATIONS_ON_DESTRUCTION
        R2_CHECK(GetTotalBytesAllocated() == 0, "We still have memory allocated!!");
#endif
        mStart = nullptr;
        mTotalSize = 0;
    }
    
    void* FreeListAllocator::Allocate(u64 size, u64 alignment, u64 offset)
    {
        const u64 allocationHeaderSize = sizeof(FreeListAllocator::AllocationHeader);
        R2_CHECK(size >= sizeof(Node), "Allocation size must be bigger than: %zu",sizeof(Node));
        R2_CHECK(alignment >= 8, "Alignment must be 8 at least");
        
        size_t padding;
        Node* affectedNode, *previousNode;
        
        Find(size, alignment, offset, padding, previousNode, affectedNode);
        
        if(affectedNode == nullptr) return nullptr;
        
        R2_CHECK(affectedNode != nullptr, "Not enough memory!");
        
        const u64 alignmentPadding = padding - allocationHeaderSize;
        const u64 requiredSize = size + padding;
        
        const u64 rest = affectedNode->data.blockSize - requiredSize;
        
        if (rest > 0)
        {
            Node* newFreeNode = (Node*)((size_t)affectedNode + requiredSize);
            newFreeNode->data.blockSize = rest;
            r2::sll::Insert(mFreeList, affectedNode, newFreeNode);
        }
        r2::sll::Remove(mFreeList, previousNode, affectedNode);
        
        const u64 headerAddress = (size_t)affectedNode + alignmentPadding;
        const u64 dataAddress = headerAddress + allocationHeaderSize;
        ((FreeListAllocator::AllocationHeader*)headerAddress)->blockSize = requiredSize;
        ((FreeListAllocator::AllocationHeader*)headerAddress)->padding = static_cast<s8>(alignmentPadding);
        
        mUsed += requiredSize;
        mPeak = std::max(mPeak, mUsed);
        
        return (void*)dataAddress;
    }
    
    void FreeListAllocator::Free(void* ptr)
    {
        const size_t currentAddress = (size_t)ptr;
        const size_t headerAddress = currentAddress - sizeof(FreeListAllocator::AllocationHeader);
        const FreeListAllocator::AllocationHeader* allocationHeader{ (FreeListAllocator::AllocationHeader*)headerAddress};
        
        Node* freeNode = (Node*)(headerAddress);
        freeNode->data.blockSize = allocationHeader->blockSize + allocationHeader->padding;
        freeNode->next = nullptr;
        
        Node* it = mFreeList.head;
        Node* itPrev = nullptr;
        while (it != nullptr)
        {
            if (ptr < it)
            {
                r2::sll::Insert(mFreeList, itPrev, freeNode);
                break;
            }
            
            itPrev = it;
            it = it->next;
        }
        
        mUsed -= freeNode->data.blockSize;
        
        Coalescence(itPrev, freeNode);
    }
    
    void FreeListAllocator::Reset(void)
    {
        mUsed = 0;
        mPeak = 0;
        Node* firstNode = (Node*)mStart;
        firstNode->data.blockSize = mTotalSize;
        firstNode->next = nullptr;
        mFreeList.head = nullptr;
        r2::sll::Insert(mFreeList, nullptr, firstNode);
    }
    
    u32 FreeListAllocator::GetAllocationSize(void* memPtr)
    {
        const size_t currentAddress = (size_t)memPtr;
        const size_t headerAddress = currentAddress - sizeof(FreeListAllocator::AllocationHeader);
        const FreeListAllocator::AllocationHeader* allocationHeader{ (FreeListAllocator::AllocationHeader*)headerAddress};
        
        return (u32)allocationHeader->blockSize - HeaderSize() - (u32)allocationHeader->padding;
    }
    
    u64 FreeListAllocator::GetTotalBytesAllocated() const
    {
        return mUsed;
    }
    
    u64 FreeListAllocator::GetTotalMemory() const
    {
        return mTotalSize;
    }
    
    u32 FreeListAllocator::HeaderSize()
    {
        return sizeof(FreeListAllocator::AllocationHeader);
    }
    
    u64 FreeListAllocator::UnallocatedBytes() const
    {
        return mTotalSize - mUsed;
    }
    
    void FreeListAllocator::Coalescence(Node* previousNode, Node* freeNode)
    {
        if (freeNode->next != nullptr &&
            (size_t)freeNode + freeNode->data.blockSize == (size_t)freeNode->next)
        {
            freeNode->data.blockSize += freeNode->next->data.blockSize;
            r2::sll::Remove(mFreeList, freeNode, freeNode->next);
        }
        
        if (previousNode != nullptr &&
            (size_t)previousNode + previousNode->data.blockSize == (size_t)freeNode)
        {
            previousNode->data.blockSize += freeNode->data.blockSize;
            r2::sll::Remove(mFreeList, previousNode, freeNode);
        }
    }
    
    void FreeListAllocator::Find(size_t size, size_t alignment, size_t offset, size_t& padding, Node*& prevNode, Node*& foundNode)
    {
        switch (mPolicy)
        {
            case FIND_FIRST:
                FindFirst(size, alignment, offset, padding, prevNode, foundNode);
                break;
            case FIND_BEST:
                FindBest(size, alignment, offset, padding, prevNode, foundNode);
                break;
            default:
                R2_CHECK(false, "Unsupported allocation policy!");
                break;
        }
    }
    
    void FreeListAllocator::FindFirst(size_t size, size_t alignment, size_t offset, size_t& padding, Node*& previousNode, Node*& foundNode)
    {
        Node* it = mFreeList.head;
        Node* itPrev = nullptr;
        
        while (it != nullptr)
        {
            byte* itAligned = (byte*)utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(it, offset + sizeof(FreeListAllocator::AllocationHeader)), alignment), offset);
            
            padding = utils::PointerOffset(it, itAligned);
            
            const size_t requiredSpace = size + padding;
            if (it->data.blockSize >= requiredSpace)
            {
                break;
            }
            
            itPrev = it;
            it = it->next;
        }
        
        previousNode = itPrev;
        foundNode = it;
    }
    
    void FreeListAllocator::FindBest(size_t size, size_t alignment, size_t offset, size_t& padding, Node*& previousNode, Node*& foundNode)
    {
        size_t smallestDiff = std::numeric_limits<size_t>::max();
        
        Node* bestBlock = nullptr;
        Node* bestBlockPrev = nullptr;
        Node* it = mFreeList.head;
        Node* itPrev = nullptr;
        
        while (it != nullptr)
        {
            byte* itAligned = (byte*)utils::PointerSubtract(utils::AlignForward(utils::PointerAdd(it, offset + sizeof(FreeListAllocator::AllocationHeader)), alignment), offset);
            padding = utils::PointerOffset(it, itAligned);
            const size_t requiredSpace = size + padding;
            
            if (it->data.blockSize >= requiredSpace && (it->data.blockSize - requiredSpace < smallestDiff))
            {
                bestBlock = it;
                bestBlockPrev = bestBlockPrev;
                smallestDiff = it->data.blockSize - requiredSpace;
            }
            
            itPrev = it;
            it = it->next;
        }
        
        previousNode = bestBlockPrev;
        foundNode = bestBlock;
    }
}

namespace r2::mem::utils
{
    FreeListArena* EmplaceFreeListArena(MemoryArea::MemorySubArea& subArea, PlacementPolicy policy, const char* file, s32 line, const char* description)
    {
        //we need to figure out how much space we have and calculate a memory boundary for the Allocator
        R2_CHECK(subArea.mBoundary.size > sizeof(FreeListArena), "subArea size(%llu) must be greater than sizeof(LinearArena)(%lu)!", subArea.mBoundary.size, sizeof(FreeListArena));
        if (subArea.mBoundary.size <= sizeof(FreeListArena))
        {
            return nullptr;
        }
        
        MemBoundary freeListAllocatorBoundary;
        freeListAllocatorBoundary.location = PointerAdd(subArea.mBoundary.location, sizeof(FreeListArena));
        freeListAllocatorBoundary.size = subArea.mBoundary.size - sizeof(FreeListArena);
        freeListAllocatorBoundary.policy = policy;
        
        FreeListArena* freeListArena = new (subArea.mBoundary.location) FreeListArena(subArea, freeListAllocatorBoundary);
        
        return freeListArena;
    }
}
