//
//  MemoryTracking.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-05.
//
#include "r2pch.h"
#include "MemoryTracking.h"
#include <cstring>
#include <stdio.h>

namespace r2
{
    namespace mem
    {
        BasicMemoryTracking::BasicMemoryTracking():mName("")
        {
        }
        
        BasicMemoryTracking::BasicMemoryTracking(const std::string& name)
        {
            mName = name;
        }
        
        void BasicMemoryTracking::SetName(const std::string& name)
        {
            mName = name;
        }
        
        BasicMemoryTracking::~BasicMemoryTracking()
        {
            Verify();
        }
        
        void BasicMemoryTracking::Verify()
        {
            auto size = mTags.size();
            
            if (size > 0)
            {
                u64 totalAskedForSize = 0;
                u64 totalAllocatedSize = 0;
                
                //@TODO(Serge): figure out a way to make loguru do just printf (without the ugly timestamp etc. log
                
                printf("=================================================================\n");
                printf("Arena Dump for: %s\n", mName.c_str());
                printf("=================================================================\n\n");
                printf("-----------------------------------------------------------------\n");
                for (decltype(size) i = 0; i < size; ++i)
                {
                    const utils::MemoryTag& tag = mTags[i];
                    
                    totalAskedForSize += tag.requestedSize;
                    totalAllocatedSize += tag.size;
                    
                    printf("Allocation: Memory address: %p, requested size: %llu, total allocated: %llu, file: %s, line: %i\n", tag.memPtr, tag.requestedSize, tag.size, tag.fileName, tag.line);
                    printf("-----------------------------------------------------------------\n");
                }
                
                if (totalAskedForSize > 0)
                {
                    printf("\n\nTotal Asked For size: %llu\n", totalAskedForSize);
                    printf("Total Allocated size: %llu\n", totalAllocatedSize);
                    printf("=================================================================\n");
                }
                
                R2_CHECK(size == 0, "We still have %lu allocations that have not been freed!", size);
            }
        }
        
        void BasicMemoryTracking::OnAllocation(const utils::MemoryTag& tag)
        {
            mTags.push_back(tag);
        }
        
        void BasicMemoryTracking::OnDeallocation(void* noptrMemory, const char* file, s32 line, const char* description)
        {
            mTags.erase(std::remove_if(mTags.begin(), mTags.end(), [noptrMemory](const utils::MemoryTag& tag)
            {
                return noptrMemory == tag.memPtr;
            }));
        }

        void BasicMemoryTracking::Reset()
        {
            mTags.clear();
        }
    }
}
