//
//  Memory.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-23.
//
#include "r2pch.h"
#include "Memory.h"
#include <cstring>
#include <sstream>
#include "r2/Core/Memory/InternalEngineMemory.h"


namespace r2
{
    namespace mem
    {
        std::vector<MemoryArea> GlobalMemory::mMemoryAreas;
        InternalEngineMemory GlobalMemory::mEngineMemory;
        
        bool GlobalMemory::Init(u64 numMemoryAreas, u64 internalEngineMemory, u64 permanentStorageSize, u64 singleFrameStorageSize)
        {
            R2_CHECK(numMemoryAreas > 0, "You can't have 0 memory areas!");
            R2_CHECK(internalEngineMemory >= permanentStorageSize, "internalEngineMemory(%llu) < permanentStorageSize(%llu", internalEngineMemory, permanentStorageSize);

            mMemoryAreas.reserve(numMemoryAreas);
            
            if (internalEngineMemory > 0)
            {
                mEngineMemory.internalEngineMemoryHandle = AddMemoryArea("Engine Memory");
                R2_CHECK(mEngineMemory.internalEngineMemoryHandle != r2::mem::MemoryArea::Invalid, "Our internal memory  storage handle is invalid!");
                
                MemoryArea* internalEngineMemoryArea = GetMemoryArea(mEngineMemory.internalEngineMemoryHandle);
                
                R2_CHECK(internalEngineMemoryArea != nullptr, "Couldn't get the internal engine storage memory area after we created it!");
                
                internalEngineMemoryArea->Init(internalEngineMemory, singleFrameStorageSize);
                
                R2_CHECK(internalEngineMemoryArea->ScratchBoundaryPtr() != nullptr, "We should have a scratch boundary!");
                
                mEngineMemory.permanentStorageHandle = internalEngineMemoryArea->AddSubArea(permanentStorageSize, "Permanent Storage");
                
                //Should be TOTAL_INTERNAL_ENGINE_MEMORY - TOTAL_INTERNAL_PERMANENT_MEMORY left in the Engine Memory Area
                
                //Setup some kind of arena for the permanent storage
                mEngineMemory.permanentStorageArena = EMPLACE_LINEAR_ARENA(*internalEngineMemoryArea->GetSubArea(mEngineMemory.permanentStorageHandle));
                
                if (singleFrameStorageSize > 0)
                {
                    mEngineMemory.singleFrameArena = EMPLACE_STACK_ARENA(*internalEngineMemoryArea->GetSubArea(internalEngineMemoryArea->ScratchSubAreaHandle()));
                }
            }
 
            return true;
        }
        
        void GlobalMemory::Shutdown()
        {
            if(mEngineMemory.permanentStorageArena)
            {
                FREE_EMPLACED_ARENA(mEngineMemory.permanentStorageArena);
            }
            
            if(mEngineMemory.singleFrameArena)
            {
                FREE_EMPLACED_ARENA(mEngineMemory.singleFrameArena);
            }
            
            for(MemoryArea& area : mMemoryAreas)
            {
                area.Shutdown();
            }

            mMemoryAreas.clear();
        }
        
        //Can be used outside of r2
        MemoryArea::Handle GlobalMemory::AddMemoryArea(const char* debugName)
        {
            R2_CHECK(mMemoryAreas.size() + 1 <= mMemoryAreas.capacity(), "We shouldn't be able to add more MemoryAreas than expected");

            if(mMemoryAreas.size() + 1 <= mMemoryAreas.capacity())
            {
                mMemoryAreas.emplace_back(debugName);
                return static_cast<MemoryArea::Handle>(mMemoryAreas.size() - 1);
            }
            
            return MemoryArea::Invalid;
        }
        
        MemoryArea* GlobalMemory::GetMemoryArea(MemoryArea::Handle handle)
        {
            if (handle != MemoryArea::Invalid && static_cast<size_t>(handle) < mMemoryAreas.size())
            {
                return &mMemoryAreas[static_cast<size_t>(handle)];
            }
            
            return nullptr;
        }
        
        namespace utils
        {
            MemoryTag::MemoryTag(const MemoryTag& tag)
            {
                line = tag.line;
                memPtr = tag.memPtr;
                alignment = tag.alignment;
                size = tag.size;
                r2::util::PathCpy(fileName, tag.fileName);
                requestedSize = tag.size;
            }
            
            MemoryTag::MemoryTag(void* memPtr, const char* fileName, u64 alignment, u64 size, s32 line)
            {
                this->line = line;
                this->memPtr = memPtr;
                this->alignment = alignment;
                this->size = size;
                r2::util::PathCpy(this->fileName, fileName);
            }
            
            MemoryTag& MemoryTag::operator=(const MemoryTag& tag)
            {
                if(&tag == this)
                {
                    return *this;
                }
                
                line = tag.line;
                memPtr = tag.memPtr;
                alignment = tag.alignment;
                size = tag.size;
                r2::util::PathCpy(fileName, tag.fileName);
                requestedSize = tag.size;
                
                return *this;
            }
        }
        
        const MemoryArea::Handle MemoryArea::Invalid;
        const MemoryArea::MemorySubArea::Handle MemoryArea::MemorySubArea::Invalid;
        const u64 MemoryArea::DefaultScratchBufferSize;
        
        MemoryArea::~MemoryArea()
        {
            Shutdown();
        }
        
        MemoryArea::MemoryArea(const char* debugName):mInitialized(false), mScratchAreaHandle(MemorySubArea::Invalid)
        {
            r2::util::PathCpy(&mDebugName[0], debugName);
        }
        
        bool MemoryArea::Init(u64 sizeInBytes, u64 scratchBufferSizeInBytes)
        {
            
            //Only initialize once
            if (mInitialized)
            {
                return mInitialized;
            }
            u64 boundarySize = sizeInBytes + scratchBufferSizeInBytes;
            
            mBoundary.location = malloc(boundarySize);
            
            R2_CHECK(mBoundary.location != nullptr, "Failed to allocate MemoryArea!");
            
            if (mBoundary.location == nullptr)
            {
                return false;
            }
            
            mBoundary.size = boundarySize;
            mCurrentNext = mBoundary.location;
            mInitialized = true;
            
            if (scratchBufferSizeInBytes > 0)
            {
                mScratchAreaHandle = AddSubArea(scratchBufferSizeInBytes, "Scratch");
                R2_CHECK(mScratchAreaHandle != Invalid, "mScratchAreaHandle is INVALID???");
            }
            
            return mInitialized;
        }
        
        MemoryArea::MemorySubArea::Handle MemoryArea::AddSubArea(u64 sizeInBytes, const std::string& subAreaName)
        {
            R2_CHECK(sizeInBytes > sizeof(uptr), "We should have more than 0 bytes");
            if(!mInitialized)
            {
                return MemoryArea::MemorySubArea::Invalid;
            }
            //first check to see if we have enough space left in the MemoryArea
            u64 bytesLeftInArea = utils::PointerOffset(mCurrentNext, utils::PointerAdd(mBoundary.location, mBoundary.size));
            
            //R2_CHECK(sizeInBytes <= bytesLeftInArea, "Requested too many bytes for this MemoryArea to hold!");
            
            if(sizeInBytes <= bytesLeftInArea)
            {
                MemorySubArea subArea;
                subArea.mBoundary.location = mCurrentNext;
                subArea.mBoundary.size = sizeInBytes;
                
                std::stringstream ss;
                if (subAreaName.size() > 0)
                {
                    ss << Name() << ": " << subAreaName << " (" << mSubAreas.size() << ")";
                }
                else
                {
                    ss << Name() << " (" << mSubAreas.size() << ")";
                }
                
                subArea.mSubArenaName = ss.str();
                mCurrentNext = utils::PointerAdd(mCurrentNext, sizeInBytes);
                
                mSubAreas.push_back(subArea);
                
                return static_cast<MemorySubArea::Handle>( mSubAreas.size() - 1 );
            }
            
            return MemorySubArea::Invalid;
        }
        
        utils::MemBoundary MemoryArea::SubAreaBoundary(MemoryArea::MemorySubArea::Handle subAreaHandle) const
        {
            if(!mInitialized || subAreaHandle == MemoryArea::MemorySubArea::Invalid)
            {
                return utils::MemBoundary();
            }
        
            utils::MemBoundary boundary;
            R2_CHECK(subAreaHandle <  static_cast<MemoryArea::MemorySubArea::Handle>(mSubAreas.size()), "We didn't get a proper MemorySubArea Handle!");
            
            if(subAreaHandle < static_cast<MemoryArea::MemorySubArea::Handle>(mSubAreas.size()))
            {
                boundary = mSubAreas[static_cast<size_t>(subAreaHandle)].mBoundary;
            }
            
            return boundary;
        }
        
        utils::MemBoundary* MemoryArea::SubAreaBoundaryPtr(MemorySubArea::Handle subAreaHandle)
        {
            if(!mInitialized || subAreaHandle == MemoryArea::MemorySubArea::Invalid)
            {
                return nullptr;
            }
            
            R2_CHECK(subAreaHandle <  static_cast<MemoryArea::MemorySubArea::Handle>(mSubAreas.size()) && subAreaHandle != MemoryArea::MemorySubArea::Invalid, "We didn't get a proper MemorySubArea Handle!");
            
            if(subAreaHandle < static_cast<MemoryArea::MemorySubArea::Handle>(mSubAreas.size()) && subAreaHandle != MemoryArea::MemorySubArea::Invalid)
            {
                return &mSubAreas[static_cast<size_t>(subAreaHandle)].mBoundary;
            }
            
            return nullptr;
        }
        
        utils::MemBoundary MemoryArea::ScratchBoundary() const
        {
            return SubAreaBoundary(mScratchAreaHandle);
        }
        
        utils::MemBoundary* MemoryArea::ScratchBoundaryPtr()
        {
            return SubAreaBoundaryPtr(mScratchAreaHandle);
        }
        
        MemoryArea::MemorySubArea* MemoryArea::GetSubArea(MemoryArea::MemorySubArea::Handle subAreaHandle)
        {
            if(!mInitialized || subAreaHandle == MemoryArea::MemorySubArea::Invalid)
            {
                return nullptr;
            }
            
            MemoryArea::MemorySubArea* subArea = nullptr;
            if(subAreaHandle < static_cast<MemoryArea::MemorySubArea::Handle>(mSubAreas.size()))
            {
                subArea = &mSubAreas[static_cast<size_t>(subAreaHandle)];
            }
            
            return subArea;
        }
        
        void MemoryArea::Shutdown()
        {
            if (mInitialized)
            {
                mSubAreas.clear();
                
                if(mBoundary.location)
                {
                    free(mBoundary.location);
                }
                
                mBoundary.location = nullptr;
                mBoundary.size = 0;
                mCurrentNext = nullptr;
                mInitialized = false;
                mScratchAreaHandle = MemorySubArea::Invalid;
            }
        }
    }
}
/* @TODO(Serge): enable one day
void * operator new(decltype(sizeof(0)) n) noexcept(false)
{
    R2_CHECK(false, "Don't use new!");
    return nullptr;
}

void operator delete(void * p) throw()
{
    R2_CHECK(false, "Don't use delete!");
}

void *operator new[](std::size_t s) noexcept(false)
{
    R2_CHECK(false, "Don't use new[]!");
    return nullptr;
}
void operator delete[](void *p) throw()
{
    R2_CHECK(false, "Don't use delete[]");
}
*/
