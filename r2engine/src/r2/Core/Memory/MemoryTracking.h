//
//  MemoryTracking.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-05.
//

#ifndef MemoryTracking_h
#define MemoryTracking_h
#include "Memory.h"

namespace r2
{
    namespace mem
    {
        class R2_API NoMemoryTracking
        {
        public:
            inline void OnAllocation(void*, u64, u64, const char* file, s32 line, const char* description) const {}
            inline void OnDeallocation(void*, const char* file, s32 line, const char* description) const {}
        
            inline u64 NumAllocations() const {return 0;}
            inline const std::vector<utils::MemoryTag>& Tags() const;
        };
        
        class R2_API BasicMemoryTracking
        {
        public:
            BasicMemoryTracking(const char* name);
            ~BasicMemoryTracking();
            void OnAllocation(void* noptrMemory, u64 size, u64 alignment, const char* file, s32 line, const char* description);
            void OnDeallocation(void* noptrMemory, const char* file, s32 line, const char* description);
            
            const std::string Name() {return mName;}
            inline u64 NumAllocations() const {return static_cast<u64>(mTags.size());}
            inline const std::vector<utils::MemoryTag>& Tags() const {return mTags;}
        private:
            char mName[Kilobytes(1)];
            //@TODO(Serge): temp - replace with static array
            std::vector<utils::MemoryTag> mTags;
        };
        
    }
}

#endif /* MemoryTracking_h */
