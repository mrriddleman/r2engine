//
//  ZipFileModifierDevice.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-19.
//
#include "r2pch.h"
#include "ZipFileModifierDevice.h"
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFile.h"

namespace r2::fs
{
    ZipFileModifierDevice::ZipFileModifierDevice()
        : FileDeviceModifier(DeviceModifier::Zip)
    {
        
    }
    
    ZipFileModifierDevice::~ZipFileModifierDevice()
    {
        R2_CHECK(moptrFilePool == nullptr, "We haven't been unmounted yet!");
    }
    
    bool ZipFileModifierDevice::Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles)
    {
        if (moptrFilePool)
        {
            return true;
        }
        
        moptrFilePool = MAKE_POOL_ARENA(permanentStorage, sizeof(ZipFile), alignof(ZipFile), numFiles);
        R2_CHECK(moptrFilePool != nullptr, "We couldn't allocate a pool of size: %llu\n", sizeof(ZipFile)*numFiles);
        
        return moptrFilePool != nullptr;
    }
    
    bool ZipFileModifierDevice::Unmount(r2::mem::LinearArena& permanentStorage)
    {
        if (moptrFilePool == nullptr)
        {
            return true;
        }
        
        FREE(moptrFilePool, permanentStorage);
        moptrFilePool = nullptr;
        
        return true;
    }
    
    File* ZipFileModifierDevice::Open(File* file)
    {
        R2_CHECK(moptrFilePool != nullptr, "You never mounted ZipFileModifierDevice!");
        if (moptrFilePool == nullptr)
        {
            R2_LOGW("Cannot open a file if we haven't been mounted yet!\n");
            return nullptr;
        }
        
        ZipFile* zipFile = ALLOC(ZipFile, *moptrFilePool);
        R2_CHECK(zipFile != nullptr, "We couldn't allocate a zip file!");
        if (zipFile != nullptr && zipFile->Open(file))
        {
            zipFile->SetFileDevice(this);
            zipFile->AddModifierConfig(GetModifier());
            return zipFile;
        }
        
        if (zipFile)
        {
            FREE(zipFile, *moptrFilePool);
        }
        
        return nullptr;
    }
    
    void ZipFileModifierDevice::Close(File* file)
    {
        R2_CHECK(moptrFilePool != nullptr, "You never mounted ZipFileModifierDevice!");
        
        if (moptrFilePool == nullptr)
        {
            R2_LOGW("Cannot close a file if we haven't been mounted yet!\n");
            return;
        }
        
        R2_CHECK(file != nullptr, "File passed in is nullptr!");
        
        if (file)
        {
            ZipFile* zipFile = static_cast<ZipFile*>(file);
            zipFile->Close();
            
            FREE(zipFile, *moptrFilePool);
        }
    }
}
