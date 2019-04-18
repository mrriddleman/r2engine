//
//  DiskFileStorageDevice.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//

#include "r2/Core/File/FileDevices/Storage/DiskFileStorageDevice.h"
#include "r2/Core/File/DiskFile.h"


namespace r2::fs
{
    DiskFileStorageDevice::DiskFileStorageDevice()
        : FileStorageDevice(DeviceStorage::Disk)
        , moptrDiskFilePool(nullptr)
    {
        
    }
    
    DiskFileStorageDevice::~DiskFileStorageDevice()
    {
        R2_CHECK(moptrDiskFilePool == nullptr, "We haven't been unmounted yet!");
    }
    
    bool DiskFileStorageDevice::Mount(r2::mem::LinearArena& permanentStorage, u64 numFilesActive)
    {
        if (moptrDiskFilePool)
        {
            return true;// we've already mounted
        }
        
        moptrDiskFilePool = MAKE_NO_CHECK_POOL_ARENA(permanentStorage, sizeof(DiskFile), numFilesActive);
        R2_CHECK(moptrDiskFilePool != nullptr, "We couldn't allocate a pool of size: %llu\n", sizeof(DiskFile)*numFilesActive);
        
        return moptrDiskFilePool != nullptr;
    }
    
    bool DiskFileStorageDevice::Unmount(r2::mem::LinearArena& permanentStorage)
    {
        if (moptrDiskFilePool == nullptr)
        {
            return true;
        }
        
        FREE(moptrDiskFilePool, permanentStorage);
        moptrDiskFilePool = nullptr;
        
        return true;
    }
    
    File* DiskFileStorageDevice::Open(const char* path, FileMode mode)
    {
        R2_CHECK(moptrDiskFilePool != nullptr, "You never mounted DiskFileStorageDevice!");
        if (moptrDiskFilePool == nullptr)
        {
            R2_LOGW("Cannot open a file if we haven't been mounted yet!\n");
            return nullptr;
        }
        
        DiskFile* diskFile = ALLOC(DiskFile, *moptrDiskFilePool);
        R2_CHECK(diskFile != nullptr, "We couldn't allocate a disk file!");
        if (diskFile != nullptr && diskFile->Open(path, mode))
        {
            diskFile->SetFileDevice(this);
            
            return diskFile;
        }
        
        return nullptr;
    }
    
    void DiskFileStorageDevice::Close(File* file)
    {
        R2_CHECK(moptrDiskFilePool != nullptr, "You never mounted DiskFileStorageDevice!");
        
        if (moptrDiskFilePool == nullptr)
        {
            R2_LOGW("Cannot close a file if we haven't been mounted yet!\n");
            return;
        }
        
        R2_CHECK(file != nullptr, "File passed in is nullptr!");
        
        if (file)
        {
            DiskFile* diskFile = static_cast<DiskFile*>(file);
            diskFile->Close();
            
            FREE(diskFile, *moptrDiskFilePool);
        }
    }
}
