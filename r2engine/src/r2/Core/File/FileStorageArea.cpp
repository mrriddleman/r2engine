//
//  FileStorageArea.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#include "r2/Core/File/FileStorageArea.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileStorageDevice.h"
#include <cstring>

namespace r2::fs
{
    FileStorageArea::FileStorageArea(const char* rootPath, u32 numFilesActive)
        : moptrStorageDevice(nullptr)
        , moptrModifiers(nullptr)
        , mNumFilesActive(numFilesActive)
    {
        R2_CHECK(rootPath != nullptr, "We got a null root!");
        strcpy(mRootPath, rootPath);
    }
    
    FileStorageArea::~FileStorageArea()
    {
        R2_CHECK(moptrStorageDevice == nullptr, "We never get unmounted!");
        R2_CHECK(moptrModifiers == nullptr, "We never get unmounted!");
    }
    
    bool FileStorageArea::Mount(r2::mem::LinearArena& storage)
    {
        if (moptrStorageDevice != nullptr)
        {
            return true; //already mounted!
        }
        
        moptrStorageDevice = ALLOC(DiskFileStorageDevice, storage);
        R2_CHECK(moptrStorageDevice != nullptr, "We couldn't allocate a DiskFileStorageDevice!\n");
        if(moptrStorageDevice)
        {
            DiskFileStorageDevice* diskDevice = static_cast<DiskFileStorageDevice*>(moptrStorageDevice);
            return diskDevice->Mount(storage, mNumFilesActive);
        }
        
        //@TODO(Serge): add in a modifier for zip
        
        return false;
    }
    
    bool FileStorageArea::Unmount(r2::mem::LinearArena& storage)
    {
        if (moptrStorageDevice == nullptr)
        {
            return true; //already unmounted
        }
        
        DiskFileStorageDevice* diskDevice = static_cast<DiskFileStorageDevice*>(moptrStorageDevice);
        bool result = diskDevice->Unmount(storage);
        R2_CHECK(result, "We couldn't unmount our storage device!");
        if (result)
        {
            FREE(moptrStorageDevice, storage);
            moptrStorageDevice = nullptr;
            return true;
        }
        
        //@TODO(Serge): add in modifiers here
        
        return false;
    }
    
}
