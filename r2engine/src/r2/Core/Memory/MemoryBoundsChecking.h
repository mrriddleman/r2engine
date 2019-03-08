//
//  MemoryBoundsChecking.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-05.
//

#ifndef MemoryBoundsChecking_h
#define MemoryBoundsChecking_h
#include <algorithm>

namespace r2
{
    namespace mem
    {
        class R2_API NoBoundsChecking
        {
        public:
            static const u64 SIZE_FRONT = 0;
            static const u64 SIZE_BACK = 0;
            
            //these should insert front and back guards into the memory
            inline void GuardFront(void*) const {}
            inline void GuardBack(void*) const {}
            
            //these should check the see if we have the correct front and back guards
            //if not then that's an issue
            inline void CheckFront(const void*) const {}
            inline void CheckBack(const void*) const {}
        };
        
        class R2_API BasicBoundsChecking
        {
        public:
            static const u64 SIZE_FRONT = sizeof(u32);
            static const u64 SIZE_BACK = sizeof(u32);
            static const u32 FRONT_GUARD = 0xABADBABE;
            static const u32 BACK_GUARD = 0xABADBABE;
            
            //these should insert front and back guards into the memory
            inline void GuardFront(void* noptrFront) const
            {
                static_cast<u32*>(noptrFront)[0] = FRONT_GUARD;
            }
            
            inline void GuardBack(void* noptrBack) const
            {
                static_cast<u32*>(noptrBack)[0] = BACK_GUARD;
            }
            
            //these should check the see if we have the correct front and back guards
            //if not then that's an issue
            inline void CheckFront(const void* noptrFront) const
            {
                R2_CHECK(static_cast<const u32*>(noptrFront)[0] == FRONT_GUARD, "Bounds Check failed! The front of the allocation was overwritten!");
            }
            
            inline void CheckBack(const void* noptrBack) const
            {
                R2_CHECK(static_cast<const u32*>(noptrBack)[0] == BACK_GUARD, "Bounds Check failed! The back of the allocation was overwritten!");
            }
        };
    }
}

#endif /* MemoryBoundsChecking_h */
