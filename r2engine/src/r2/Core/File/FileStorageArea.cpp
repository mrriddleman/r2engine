//
//  FileStorageArea.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#include "r2/Core/File/FileStorageArea.h"

#include "r2/Core/File/DirectoryUtils.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileStorageDevice.h"
#include "r2/Core/File/FileDevices/Modifiers/Safe/SafeFileModifierDevice.h"
#include "r2/Core/File/FileDevices/Modifiers/Zip/ZipFileModifierDevice.h"
#include "r2/Core/File/File.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include <cstring>
#include <cstdio>

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
        bool mounted = false;
        moptrStorageDevice = ALLOC(DiskFileStorageDevice, storage);
        R2_CHECK(moptrStorageDevice != nullptr, "We couldn't allocate a DiskFileStorageDevice!\n");
        if(moptrStorageDevice)
        {
            DiskFileStorageDevice* diskDevice = static_cast<DiskFileStorageDevice*>(moptrStorageDevice);
            mounted = diskDevice->Mount(storage, mNumFilesActive);
        }
        
        moptrModifiers = MAKE_SARRAY(storage, FileDeviceModifier*, 10);
        
        R2_CHECK(moptrModifiers != nullptr, "We should have a file device modifier list created");
        
        DiskFileStorageDevice* storageDevice = static_cast<DiskFileStorageDevice*>(moptrStorageDevice);
        
        FileDeviceModifier* safeMod = (FileDeviceModifier*)ALLOC_PARAMS(SafeFileModifierDevice, storage, *storageDevice);
        
        if (safeMod)
        {
            mounted = safeMod->Mount(storage, mNumFilesActive);
            r2::sarr::Push(*moptrModifiers, safeMod);
        }
        
        FileDeviceModifier* zipMod = (FileDeviceModifier*)ALLOC(ZipFileModifierDevice, storage);
        
        if (zipMod)
        {
            mounted = zipMod->Mount(storage, mNumFilesActive);
            r2::sarr::Push(*moptrModifiers, zipMod);
        }
        
        
        return mounted;
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
        }
        
        auto size = r2::sarr::Size(*moptrModifiers);
        
        for (u64 i = 0; i < size; ++i)
        {
            FileDeviceModifier* mod = r2::sarr::At(*moptrModifiers, i);
            mod->Unmount(storage);
            FREE(mod, storage);
        }
        
        r2::sarr::Clear(*moptrModifiers);
        
        FREE(moptrModifiers, storage);
        moptrModifiers = nullptr;
        
        return true;
    }
    
    bool FileStorageArea::FileExists(const char* filePath)
    {
        if (moptrStorageDevice == nullptr)
        {
            return false; //already unmounted
        }
        
        R2_CHECK(filePath != nullptr, "File Path is nullptr");

        if (filePath)
        {
            DiskFileStorageDevice* diskDevice = static_cast<DiskFileStorageDevice*>(moptrStorageDevice);
            File* file = diskDevice->Open(filePath, Mode::Read);
            if (file)
            {
                diskDevice->Close(file);
                return true;
            }
        }
        
        return false;
    }
    
    bool FileStorageArea::DeleteFile(const char* filePath)
    {
        R2_CHECK(filePath != nullptr, "File Path is nullptr");
        
        if (filePath)
        {
            return std::remove(filePath) == 0;
        }
        
        return false;
    }
    
    bool FileStorageArea::RenameFile(const char* existingFile, const char* newName)
    {
        R2_CHECK(existingFile != nullptr, "existingFile is nullptr");
        R2_CHECK(newName != nullptr, "newName is nullptr");
        
        if (existingFile && newName)
        {
            return std::rename(existingFile, newName) == 0;
        }
        
        return false;
    }
    
    bool FileStorageArea::CopyFile(const char* existingFile, const char* newName)
    {
        R2_CHECK(existingFile != nullptr, "existingFile is nullptr");
        R2_CHECK(newName != nullptr, "newName is nullptr");
        
        bool success = false;
        
        if (existingFile && newName)
        {
            DiskFileStorageDevice* diskDevice = static_cast<DiskFileStorageDevice*>(moptrStorageDevice);
            
            FileMode readMode;
            readMode |= Mode::Read;
            readMode |= Mode::Binary;
            
            FileMode writeMode;
            writeMode |= Mode::Write;
            writeMode |= Mode::Binary;
            
            File* readFile = diskDevice->Open(existingFile, readMode);
            File* writeFile = diskDevice->Open(newName, writeMode);
            
            R2_CHECK(readFile != nullptr, "We should have a read file");
            R2_CHECK(writeFile != nullptr, "We should have a write file");
            
            if (readFile && writeFile)
            {
                u64 sizeOfFile = readFile->Size();
                byte* fileData = nullptr;
                
                fileData = ALLOC_BYTES(*MEM_ENG_SCRATCH_PTR, sizeOfFile, sizeof(byte), __FILE__, __LINE__, "");
                
                if(readFile->ReadAll(fileData))
                {
                    if( writeFile->Write(fileData, sizeOfFile) )
                    {
                        success = true;
                    }
                }
                
                FREE(fileData, *MEM_ENG_SCRATCH_PTR);
            }
            
            if (readFile)
            {
                diskDevice->Close(readFile);
            }
            
            if (writeFile)
            {
                diskDevice->Close(writeFile);
            }
        }
        
        return success;
    }
    
    void FileStorageArea::CreateFileListFromDirectory(const char* directory, const char* ext, r2::SArray<char[r2::fs::FILE_PATH_LENGTH]>* fileList)
    {
        dir::CreateFileListFromDirectory(directory, ext, fileList);
    }
}
