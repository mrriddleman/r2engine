//
//  MemoryTagging.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-05.
//

#ifndef MemoryTagging_h
#define MemoryTagging_h

namespace r2
{
    namespace mem
    {
        class R2_API NoMemoryTagging
        {
        public:
            inline void TagAllocation(void*, u64) const {}
            inline void TagDeallocation(void*, u64) const {}
        };
        
        
        class R2_API BasicMemoryTagging
        {
          
        public:
            
            static const u8 ALLOCATION_PATTERN = 0xCD;
            static const u8 DEALLOCATION_PATTERN = 0xDD;
            
            inline void TagAllocation(void* ptrMem, u64 size) const
            {
                memset((byte*)ptrMem, ALLOCATION_PATTERN, size);
            }
            
            inline void TagDeallocation(void* ptrMem, u64 size) const
            {
                memset((byte*)ptrMem, DEALLOCATION_PATTERN, size);
            }
        };
    }
}

#endif /* MemoryTagging_h */
