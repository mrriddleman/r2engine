//
//  LinearAllocator.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-11.
//

#ifndef LinearAllocator_h
#define LinearAllocator_h
#include "r2/Core/Memory/Memory.h"

/*Based on:
 https://blog.molecular-matters.com/2012/08/14/memory-allocation-strategies-a-linear-allocator/
 */

namespace r2
{
    namespace mem
    {
        class LinearAllocator
        {
        public:
            explicit LinearAllocator(const utils::MemBoundary& boundary);
            void* Allocate(u64 size, u64 alignment, u64 offset);
            void Free(void* ptr);
            void Reset(void);
            
            u32 GetAllocationSize(void* memoryPtr) const;
            
            u64 GetTotalBytesAllocated() const;
            
        private:
            void* mStart;
            void* mEnd;
            void* mCurrent;
        };
    }
}

#endif /* LinearAllocator_h */
