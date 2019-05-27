//
//  LinearAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-11.
//

#ifndef LinearAllocator_h
#define LinearAllocator_h
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/MemoryBoundsChecking.h"
#include "r2/Core/Memory/MemoryTagging.h"
#include "r2/Core/Memory/MemoryTracking.h"
#include "r2/Platform/Platform.h"
/*Based on:
 https://blog.molecular-matters.com/2012/08/14/memory-allocation-strategies-a-linear-allocator/
 */

#define MAKE_LINEARA(arena, capacity) r2::mem::utils::CreateLinearAllocator(arena, capacity, __FILE__, __LINE__, "")

#define MAKE_LINEAR_ARENA(arena, capacity) r2::mem::utils::CreateLinearArena(arena, capacity, __FILE__, __LINE__, "")

#define EMPLACE_LINEAR_ARENA(subarea) r2::mem::utils::EmplaceLinearArena(subarea, __FILE__, __LINE__, "")

namespace r2
{
    namespace mem
    {
        class LinearAllocator
        {
        public:
            explicit LinearAllocator(const utils::MemBoundary& boundary);
            
            ~LinearAllocator();

            void* Allocate(u64 size, u64 alignment, u64 offset);
            void Free(void* ptr);
            void Reset(void);
            
            u32 GetAllocationSize(void* memoryPtr) const;
            
            inline u64 GetTotalBytesAllocated() const {return utils::PointerOffset(mStart, mCurrent);}
            inline u64 GetTotalMemory() const {return utils::PointerOffset(mStart, mEnd);}
            inline const void* StartPtr() const {return mStart;}
            static inline u32 HeaderSize() {return sizeof(utils::Header);}
            inline u64 UnallocatedBytes() const {return utils::PointerOffset(mCurrent, mEnd);}
            
        private:
            byte* mStart;
            byte* mEnd;
            byte* mCurrent;
        };
        
        //@TODO(Serge): Add in more verbose tracking etc
#if defined(R2_DEBUG) || defined(R2_RELEASE)
        typedef MemoryArena<LinearAllocator, SingleThreadPolicy, BasicBoundsChecking, BasicMemoryTracking, BasicMemoryTagging> LinearArena;
#else
        typedef MemoryArena<LinearAllocator, SingleThreadPolicy, NoBoundsChecking, NoMemoryTracking, NoMemoryTagging> LinearArena;
#endif
    }
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::LinearAllocator* CreateLinearAllocator(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    
    template<class ARENA> r2::mem::LinearArena* CreateLinearArena(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description);
    
    LinearArena* EmplaceLinearArena(MemoryArea::MemorySubArea& subArea, const char* file, s32 line, const char* description);
}

namespace r2::mem::utils
{
    template<class ARENA> r2::mem::LinearAllocator* CreateLinearAllocator(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
    {
        void* linearAllocatorStartPtr = ALLOC_BYTES(arena, sizeof(LinearAllocator) + capacity, 64, file, line, description);
        
        R2_CHECK(linearAllocatorStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(linearAllocatorStartPtr, sizeof(LinearAllocator));
        
        utils::MemBoundary linearBoundary;
        linearBoundary.location = boundaryStart;
        linearBoundary.size = capacity;
        
        LinearAllocator* linearAllocator = new (linearAllocatorStartPtr) LinearAllocator(linearBoundary);
        
        R2_CHECK(linearAllocator != nullptr, "Couldn't placement new?");
        
        return linearAllocator;
    }
    
    template<class ARENA> r2::mem::LinearArena* CreateLinearArena(ARENA& arena, u64 capacity, const char* file, s32 line, const char* description)
    {
        void* linearArenaStartPtr = ALLOC_BYTES(arena, sizeof(LinearArena) + capacity, CPLAT.CPUCacheLineSize(), file, line, description);
        
        R2_CHECK(linearArenaStartPtr != nullptr, "We shouldn't have null pool!");
        
        void* boundaryStart = r2::mem::utils::PointerAdd(linearArenaStartPtr, sizeof(LinearArena));
        
        utils::MemBoundary linearBoundary;
        linearBoundary.location = boundaryStart;
        linearBoundary.size = capacity;
        
        LinearArena* linearArena = new (linearArenaStartPtr) LinearArena(linearBoundary);
        
        R2_CHECK(linearArena != nullptr, "Couldn't placement new?");
        
        return linearArena;
    }
    
    
}

#endif /* LinearAllocator_h */
