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

#define MAKE_POOLA(arena, elementSize, capacity) r2::mem::utils::CreatePoolAllocator(arena, elementSize, capacity, __FILE__, __LINE__, "")

#define MAKE_POOL_ARENA(arena, elementSize, capacity) r2::mem::utils::CreatePoolArena(arena, elementSize, capacity, __FILE__, __LINE__, "")

#define MAKE_NO_CHECK_POOL_ARENA(arena, elementSize, capacity) r2::mem::utils::CreateNoCheckPoolArena(arena, elementSize, capacity, __FILE__, __LINE__, "")

#define EMPLACE_POOL_ARENA(subarea) r2::mem::utils::EmplacePoolArena(subarea, __FILE__, __LINE__, "")

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
            static inline u32 HeaderSize() {return 0;}
            inline u64 UnallocatedBytes() const {return (TotalElements() - mNumAllocations)*mElementSize;}
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
            
            byte* mStart;
            byte* mEnd;
            const u64 mElementSize;
            const u64 mAlignment;
            u64 mNumAllocations;
        };
        
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        typedef MemoryArena<PoolAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> PoolArena;
        
        typedef MemoryArena<PoolAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> NoCheckPoolArena;
#else
        typedef MemoryArena<PoolAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> PoolArena;
        
        typedef MemoryArena<PoolAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> NoCheckPoolArena;
#endif
    }
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::PoolAllocator* CreatePoolAllocator(ARENA& arena, u64 elementSize, u64 capacity, const char* file, s32 line, const char* description);
    
    template<class ARENA> r2::mem::PoolArena* CreatePoolArena(ARENA& arena, u64 elementSize, u64 capacity, const char* file, s32 line, const char* description);
    
    template<class ARENA> r2::mem::NoCheckPoolArena* CreateNoCheckPoolArena(ARENA& arena, u64 elementSize, u64 capacity, const char* file, s32 line, const char* description);
    
    PoolArena* EmplacePoolArena(MemoryArea::MemorySubArea& subArea, u64 elementSize, const char* file, s32 line, const char* description);
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::PoolAllocator* CreatePoolAllocator(ARENA& arena, u64 elementSize, u64 capacity, const char* file, s32 line, const char* description)
    {
        u64 poolSizeInBytes = capacity * elementSize;
        
        void* poolAllocatorStartPtr = ALLOC_BYTES(arena, sizeof(PoolAllocator) + poolSizeInBytes, elementSize, file, line, description);
        
        R2_CHECK(poolAllocatorStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(poolAllocatorStartPtr, sizeof(PoolAllocator));
        
        utils::MemBoundary poolBoundary;
        poolBoundary.location = boundaryStart;
        poolBoundary.size = poolSizeInBytes;
        poolBoundary.elementSize = elementSize;
        poolBoundary.alignment = elementSize;
        poolBoundary.offset = 0; //?
        
        PoolAllocator* pool = new (poolAllocatorStartPtr) PoolAllocator(poolBoundary);
        
        R2_CHECK(pool != nullptr, "Couldn't placement new?");
        
        return pool;
    }
    
    template<class ARENA> r2::mem::PoolArena* CreatePoolArena(ARENA& arena, u64 elementSize, u64 capacity, const char* file, s32 line, const char* description)
    {
        
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        elementSize = elementSize + BasicBoundsChecking::SIZE_BACK + BasicBoundsChecking::SIZE_FRONT;
#endif
        u64 poolSizeInBytes = capacity * elementSize;
        
        void* poolArenaStartPtr = ALLOC_BYTES(arena, sizeof(PoolArena) + poolSizeInBytes, alignof(elementSize), file, line, description);
        
        R2_CHECK(poolArenaStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(poolArenaStartPtr, sizeof(PoolArena));
        
        utils::MemBoundary poolBoundary;
        poolBoundary.location = boundaryStart;
        poolBoundary.size = poolSizeInBytes;
        poolBoundary.elementSize = elementSize;
        poolBoundary.alignment = elementSize;
        poolBoundary.offset = 0; //?

        PoolArena* pool = new (poolArenaStartPtr) PoolArena(poolBoundary);
        
        R2_CHECK(pool != nullptr, "Couldn't placement new?");
        
        return pool;
    }
    
    template<class ARENA> r2::mem::NoCheckPoolArena* CreateNoCheckPoolArena(ARENA& arena, u64 elementSize, u64 capacity, const char* file, s32 line, const char* description)
    {
        u64 poolSizeInBytes = capacity * elementSize;
        
        void* poolArenaStartPtr = ALLOC_BYTES(arena, sizeof(NoCheckPoolArena) + poolSizeInBytes, alignof(elementSize), file, line, description);
        
        R2_CHECK(poolArenaStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(poolArenaStartPtr, sizeof(NoCheckPoolArena));
        
        utils::MemBoundary poolBoundary;
        poolBoundary.location = boundaryStart;
        poolBoundary.size = poolSizeInBytes;
        poolBoundary.elementSize = elementSize;
        poolBoundary.alignment = elementSize;
        poolBoundary.offset = 0; //?
        
        NoCheckPoolArena* pool = new (poolArenaStartPtr) NoCheckPoolArena(poolBoundary);
        
        R2_CHECK(pool != nullptr, "Couldn't placement new?");
        
        return pool;
    }
}

#endif /* PoolAllocator_h */
