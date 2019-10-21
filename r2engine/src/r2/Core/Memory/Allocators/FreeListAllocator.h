//
//  FreeListAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-10-15.
//

#ifndef FreeListAllocator_h
#define FreeListAllocator_h

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SinglyLinkedList.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Core/Memory/MemoryTagging.h"
#include "r2/Core/Memory/MemoryTracking.h"
#include "r2/Platform/Platform.h"

/*
 Based on: https://github.com/mtrebi/memory-allocators/blob/master/includes/FreeListAllocator.h
 */

#define MAKE_FREELISTA(arena, capacity) r2::mem::utils::CreateFreeListAllocator(arena, capacity, __FILE__, __LINE__, "")

#define MAKE_FREELIST_ARENA(arena, capacity) r2::mem::utils::CreateFreeListArena(arena, capacity, __FILE__, __LINE__, "")

#define EMPLACE_FREELIST_ARENA(subarea) r2::mem::utils::EmplaceFreeListArena(subarea, __FILE__, __LINE__, "")

namespace r2::mem
{
    enum PlacementPolicy
    {
        FIND_FIRST,
        FIND_BEST
    };
    
    class FreeListAllocator
    {
    public:
        
        struct FreeHeader
        {
            size_t blockSize;
        };
        
        struct AllocationHeader
        {
            size_t blockSize;
            char padding;
        };
        
        explicit FreeListAllocator(const utils::MemBoundary& boundary);
        
        ~FreeListAllocator();
        
        void* Allocate(u64 size, u64 alignment, u64 offset);
        void Free(void* ptr);
        void Reset(void);
        
        u32 GetAllocationSize(void* memPtr);
        
        u64 GetTotalBytesAllocated() const;
        u64 GetTotalMemory() const;
        inline const void* StartPtr() const {return mStart;}
        static u32 HeaderSize();
        u64 UnallocatedBytes() const;
        
    private:
        
        
        typedef typename SinglyLinkedList<FreeHeader>::Node Node;
        
        void Coalescence(Node* prevBlock, Node* freeBlock);
        void Find(size_t size, size_t alignment, size_t offset, size_t& padding, Node*& prevNode, Node*& foundNode);
        void FindFirst(size_t size, size_t alignment, size_t offset, size_t& padding, Node*& prevNode, Node*& foundNode);
        void FindBest(size_t size, size_t alignment, size_t offset, size_t& padding, Node*& prevNode, Node*& foundNode);
        
        byte* mStart;
        u64 mTotalSize;
        u64 mUsed;
        u64 mPeak;
        SinglyLinkedList<FreeHeader> mFreeList;
        PlacementPolicy mPolicy;
        
    };
    
#if defined(R2_DEBUG) || defined(R2_RELEASE)
    typedef MemoryArena<FreeListAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> FreeListArena;
#else
    typedef MemoryArena<FreeListAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> FreeListArena;
#endif
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::FreeListAllocator* CreateFreeListAllocator(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    
    template<class ARENA> r2::mem::FreeListArena* CreateFreeListArena(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    
    FreeListArena* EmplaceFreeListArena(MemoryArea::MemorySubArea& subArea, const char* file, s32 line, const char* description);
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::FreeListAllocator* CreateFreeListAllocator(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
    {
        void* freeListAllocatorStartPtr = ALLOC_BYTES(arena, sizeof(FreeListAllocator) + capacity, CPLAT.CPUCacheLineSize(), file, line, description);
        
        R2_CHECK(freeListAllocatorStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(freeListAllocatorStartPtr, sizeof(FreeListAllocator));
        
        utils::MemBoundary boundary;
        boundary.location = boundaryStart;
        boundary.size = capacity;
        
        FreeListAllocator* freeListAllocator = new (freeListAllocatorStartPtr) FreeListAllocator(boundary);
        
        R2_CHECK(freeListAllocator != nullptr, "Couldn't placement new?");
        
        return freeListAllocator;
    }
    
    template<class ARENA> r2::mem::FreeListArena* CreateFreeListArena(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
    {
        void* freeListArenaStartPtr = ALLOC_BYTES(arena, sizeof(FreeListArena) + capacity, CPLAT.CPUCacheLineSize(), file, line, description);
        
        R2_CHECK(freeListArenaStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(freeListArenaStartPtr, sizeof(FreeListArena));
        
        utils::MemBoundary boundary;
        boundary.location = boundaryStart;
        boundary.size = capacity;
        
        FreeListArena* freeListArena = new (freeListArenaStartPtr) FreeListArena(boundary);
        
        R2_CHECK(freeListArena != nullptr, "Couldn't placement new?");
        
        return freeListArena;
    }
}

#endif /* FreeListAllocator_h */
