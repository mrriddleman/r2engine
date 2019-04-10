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

#define MAKE_STACKA(arena, capacity) r2::mem::utils::CreateStackAllocator(arena, capacity, __FILE__, __LINE__, "")

#define MAKE_STACK_ARENA(arena, capacity) r2::mem::utils::CreateStackArena(arena, capacity, __FILE__, __LINE_, "")

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
            u32 HeaderSize() const;
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

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::StackAllocator* CreateStackAllocator(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    
    template<class ARENA> r2::mem::StackArena* CreateStackArena(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    
    StackArena* EmplaceStackArena(MemoryArea::MemorySubArea& subArea, const char* file, s32 line, const char* description);
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::StackAllocator* CreateStackAllocator(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
    {
        void* stackAllocatorStartPtr = ALLOC_BYTES(arena, sizeof(StackAllocator) + capacity, 64, file, line, description);
        
        R2_CHECK(stackAllocatorStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(stackAllocatorStartPtr, sizeof(StackAllocator));
        
        utils::MemBoundary stackBoundary;
        stackBoundary.location = boundaryStart;
        stackBoundary.size = capacity;

        StackAllocator* stack = new (stackAllocatorStartPtr) StackAllocator(stackBoundary);
        
        R2_CHECK(stack != nullptr, "Couldn't placement new?");
        
        return stack;
    }
    
    template<class ARENA> r2::mem::StackArena* CreateStackArena(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
    {
        void* stackArenaStartPtr = ALLOC_BYTES(arena, sizeof(StackArena) + capacity, 64, file, line, description);
        
        R2_CHECK(stackArenaStartPtr != nullptr, "We shouldn't have null ");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(stackArenaStartPtr, sizeof(StackArena));
        
        utils::MemBoundary linearBoundary;
        linearBoundary.location = boundaryStart;
        linearBoundary.size = capacity;
        
        StackArena* stackArena = new (stackArenaStartPtr) StackArena(linearBoundary);
        
        R2_CHECK(stackArena != nullptr, "Couldn't placement new?");
        
        return stackArena;
    }
}

#endif /* StackAllocator_h */
