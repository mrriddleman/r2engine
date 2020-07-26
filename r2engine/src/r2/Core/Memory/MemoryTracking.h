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
            inline void OnAllocation(const utils::MemoryTag& tag) const {}
            inline void OnDeallocation(void*, const char* file, s32 line, const char* description) const {}
        
            inline u64 NumAllocations() const {return 0;}
            inline const std::vector<utils::MemoryTag>& Tags() const
            {
                static std::vector<utils::MemoryTag> tags;
                return tags;
            }
            void SetName(const std::string& name){}
            
            void Verify(){}
            void Reset() {}
        };
        
        class R2_API BasicMemoryTracking
        {
        public:
            BasicMemoryTracking();
            explicit BasicMemoryTracking(const std::string& name);
            ~BasicMemoryTracking();
            void OnAllocation(const utils::MemoryTag& tag);
            void OnDeallocation(void* noptrMemory, const char* file, s32 line, const char* description);
            void Verify();
            void Reset();
            void SetName(const std::string& name);
            
            const std::string Name() {return mName;}
            inline u64 NumAllocations() const {return static_cast<u64>(mTags.size());}
            inline const std::vector<utils::MemoryTag>& Tags() const {return mTags;}
        private:
            std::string mName;
            std::vector<utils::MemoryTag> mTags;
        };
        
    }
}

#endif /* MemoryTracking_h */
