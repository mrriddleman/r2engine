//
//  SafeModifierDevice.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-22.
//

#include "SafeFileModifierDevice.h"
#include "r2/Core/File/FileDevices/Modifiers/Safe/SafeFile.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"

namespace r2::fs
{
    SafeFileModifierDevice::SafeFileModifierDevice(DiskFileStorageDevice& storageDevice)
        : FileDeviceModifier(DeviceModifier::Safe)
        , mStorageDevice(storageDevice)
    {
        
    }
    
    SafeFileModifierDevice::~SafeFileModifierDevice()
    {
        R2_CHECK(moptrFilePool == nullptr, "We haven't been unmounted yet!");
    }
    
    bool SafeFileModifierDevice::Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles)
    {
        if (moptrFilePool)
        {
            return true;
        }
        
        moptrFilePool = MAKE_NO_CHECK_POOL_ARENA(permanentStorage, sizeof(SafeFile), numFiles);
        R2_CHECK(moptrFilePool != nullptr, "We couldn't allocate a pool of size: %llu\n", sizeof(SafeFile)*numFiles);
        
        return moptrFilePool != nullptr;
    }
    
    bool SafeFileModifierDevice::Unmount(r2::mem::LinearArena& permanentStorage)
    {
        if (moptrFilePool == nullptr)
        {
            return true;
        }
        
        FREE(moptrFilePool, permanentStorage);
        moptrFilePool = nullptr;
        
        return true;
    }
    
    File* SafeFileModifierDevice::Open(File* file)
    {
        R2_CHECK(moptrFilePool != nullptr, "You never mounted SafeFileModifierDevice!");
        if (moptrFilePool == nullptr)
        {
            R2_LOGW("Cannot open a file if we haven't been mounted yet!\n");
            return nullptr;
        }
        
        SafeFile* safeFile = ALLOC_PARAMS(SafeFile, *moptrFilePool, mStorageDevice);
        R2_CHECK(safeFile != nullptr, "We couldn't allocate a safe file!");
        if (safeFile != nullptr && safeFile->Open(file))
        {
            safeFile->SetFileDevice(this);
            safeFile->AddModifierConfig(GetModifier());
            return safeFile;
        }
        
        return nullptr;
    }
    
    void SafeFileModifierDevice::Close(File* file)
    {
        R2_CHECK(moptrFilePool != nullptr, "You never mounted DiskFileStorageDevice!");
        
        if (moptrFilePool == nullptr)
        {
            R2_LOGW("Cannot close a file if we haven't been mounted yet!\n");
            return;
        }
        
        R2_CHECK(file != nullptr, "File passed in is nullptr!");
        
        if (file)
        {
            SafeFile* safeFile = static_cast<SafeFile*>(file);
            safeFile->Close();
            
            FREE(safeFile, *moptrFilePool);
        }
    }
}
