//
//  MemoryTracking.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-05.
//

#include "MemoryTracking.h"
#include <cstring>

namespace r2
{
    namespace mem
    {
        BasicMemoryTracking::BasicMemoryTracking()
        {
            strcpy(mName, "");
        }
        
        BasicMemoryTracking::BasicMemoryTracking(const char* name)
        {
            strcpy(mName, name);
        }
        
        BasicMemoryTracking::~BasicMemoryTracking()
        {
            R2_CHECK(mTags.size() == 0, "We still have Memory allocations that have not been freed!");
        }
        
        void BasicMemoryTracking::OnAllocation(void* noptrMemory, u64 size, u64 alignment, const char* file, s32 line, const char* description)
        {
            mTags.emplace_back(noptrMemory, file, alignment, size, line);
        }
        
        void BasicMemoryTracking::OnDeallocation(void* noptrMemory, const char* file, s32 line, const char* description)
        {
            mTags.erase(std::remove_if(mTags.begin(), mTags.end(), [noptrMemory](const utils::MemoryTag& tag)
            {
                return noptrMemory == tag.memPtr;
            }));
        }
    }
}
