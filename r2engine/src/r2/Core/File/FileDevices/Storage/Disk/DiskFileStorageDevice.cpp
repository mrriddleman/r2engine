//
//  DiskFileStorageDevice.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//
#include "r2pch.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileStorageDevice.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"


namespace r2::fs
{
    DiskFileStorageDevice::DiskFileStorageDevice()
        : FileStorageDevice(DeviceStorage::Disk)
    {
        
    }
    
    DiskFileStorageDevice::~DiskFileStorageDevice()
    {
        R2_CHECK(moptrFilePool == nullptr, "We haven't been unmounted yet!");
    }
    
    bool DiskFileStorageDevice::Mount(r2::mem::LinearArena& permanentStorage, u64 numFilesActive)
    {
        if (moptrFilePool)
        {
            return true;// we've already mounted
        }
        
        moptrFilePool = MAKE_POOL_ARENA(permanentStorage, sizeof(DiskFile), numFilesActive);
        R2_CHECK(moptrFilePool != nullptr, "We couldn't allocate a pool of size: %llu\n", sizeof(DiskFile)*numFilesActive);
        
        return moptrFilePool != nullptr;
    }
    
    bool DiskFileStorageDevice::Unmount(r2::mem::LinearArena& permanentStorage)
    {
        if (moptrFilePool == nullptr)
        {
            return true;
        }
        
        FREE(moptrFilePool, permanentStorage);
        moptrFilePool = nullptr;
        
        return true;
    }
    
    File* DiskFileStorageDevice::Open(const char* path, FileMode mode)
    {
        R2_CHECK(moptrFilePool != nullptr, "You never mounted DiskFileStorageDevice!");
        if (moptrFilePool == nullptr)
        {
            R2_LOGW("Cannot open a file if we haven't been mounted yet!\n");
            return nullptr;
        }
        
        DiskFile* diskFile = ALLOC(DiskFile, *moptrFilePool);
        R2_CHECK(diskFile != nullptr, "We couldn't allocate a disk file!");
        if (diskFile != nullptr && diskFile->Open(path, mode))
        {
            diskFile->SetFileDevice(this);
            diskFile->SetStorageDeviceConfig(GetStorageType());
            return diskFile;
        }
        
        if (diskFile)
        {
            FREE(diskFile, *moptrFilePool);
        }
        
        return nullptr;
    }
    
    void DiskFileStorageDevice::Close(File* file)
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
            DiskFile* diskFile = static_cast<DiskFile*>(file);
            diskFile->Close();
            
            FREE(diskFile, *moptrFilePool);
        }
    }
}
