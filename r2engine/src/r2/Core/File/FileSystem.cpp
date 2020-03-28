//
//  FileSystem.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-15.
//
#include "r2pch.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/FileStorageArea.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/File/FileSystemInternal.h"

namespace r2::fs
{
    const char* FileSystem::ALL_EXT = "*";
    
    r2::SArray<FileStorageArea*>* FileSystem::mStorageAreas = nullptr;
    
    bool FileSystem::Init(r2::mem::LinearArena& arena, u32 storageAreaCapacity)
    {
        R2_CHECK(storageAreaCapacity > 0, "We must have at least 1 storage area");
        mStorageAreas = MAKE_SARRAY(arena, FileStorageArea*, storageAreaCapacity);
        R2_CHECK(mStorageAreas != nullptr, "We couldn't create mStorageAreas static array!");
        return mStorageAreas != nullptr;
    }
    
    void FileSystem::Mount(FileStorageArea& storageArea)
    {
        R2_CHECK(mStorageAreas != nullptr, "mStorageAreas cannot be null");
        if (mStorageAreas != nullptr)
        {
            r2::sarr::Push(*mStorageAreas, &storageArea);
        }
    }
    
    void FileSystem::Shutdown(r2::mem::LinearArena& arena)
    {
        UnmountStorageAreas();
        FREE(mStorageAreas, arena);
        mStorageAreas = nullptr;
    }
    
    File* FileSystem::Open(const DeviceConfig& config, const char* path, FileMode mode)
    {
        FileStorageArea* storageArea = FindStorageArea(path);
        
        if (storageArea)
        {
            FileSystemInternal fs(*storageArea);
            return fs.Open(config, path, mode);
        }
        
        return nullptr;
    }
    
    void FileSystem::Close(File* file)
    {
        R2_CHECK(file != nullptr, "File is nullptr!");
        
        if (file)
        {
            FileStorageDevice* device = file->GetFileDevice();
            R2_CHECK(device != nullptr, "We should have found the device for this file");
            
            //This is relying on the fact that the FileDeviceModifiers will call close on their files
            device->Close(file);
            
            //@TODO(Serge): have the file storage area for this file commit?
            
        }
    }
    
    bool FileSystem::FileExists(const char* path)
    {
        R2_CHECK(path != nullptr, "You passed nullptr to path");
        if (!path)
        {
            return false;
        }
        
        FileStorageArea* storageArea = FindStorageArea(path);
        if (storageArea)
        {
            return storageArea->FileExists(path);
        }
        
        return false;
    }
    
    bool FileSystem::DeleteFile(const char* path)
    {
        R2_CHECK(path != nullptr, "You passed nullptr to path");
        if (!path)
        {
            return false;
        }
        
        FileStorageArea* storageArea = FindStorageArea(path);
        if (storageArea)
        {
            return storageArea->DeleteFile(path);
        }
        
        return false;
    }
    
    bool FileSystem::RenameFile(const char* existingFile, const char* newName)
    {
        R2_CHECK(existingFile != nullptr, "You passed nullptr for existingFile");
        R2_CHECK(newName != nullptr, "You passed nullptr for newName");
        
        if (!existingFile || !newName)
        {
            return false;
        }
        
        FileStorageArea* storageArea1 = FindStorageArea(existingFile);
        FileStorageArea* storageArea2 = FindStorageArea(newName);
        
        R2_CHECK(storageArea1 == storageArea2, "We don't have inter-storage area rename right now");
        
        if (storageArea1 == storageArea2)
        {
            return storageArea1->RenameFile(existingFile, newName);
        }
        
        return false;
    }
    
    bool FileSystem::CopyFile(const char* existingFile, const char* newName)
    {
        R2_CHECK(existingFile != nullptr, "You passed nullptr for existingFile");
        R2_CHECK(newName != nullptr, "You passed nullptr for newName");
        
        if (!existingFile || !newName)
        {
            return false;
        }
        
        FileStorageArea* storageArea1 = FindStorageArea(existingFile);
        FileStorageArea* storageArea2 = FindStorageArea(newName);
        
        R2_CHECK(storageArea1 == storageArea2, "We don't have inter-storage area copy right now");
        
        if (storageArea1 == storageArea2)
        {
            return storageArea1->CopyFile(existingFile, newName);
        }
        
        return false;
    }
    
    bool FileSystem::DirExists(const char* path)
    {
        return false;
    }
    
    bool FileSystem::CreateDirectory(const char* path, bool recursive)
    {
        return false;
    }
    
    bool FileSystem::DeleteDirectory(const char* path)
    {
        return false;
    }

    void FileSystem::CreateFileListFromDirectory(const char* directory, const char* ext, r2::SArray<char[r2::fs::FILE_PATH_LENGTH]>* fileList)
    {
        FileStorageArea* storageArea = FindStorageArea(directory);
        if (storageArea)
        {
            storageArea->CreateFileListFromDirectory(directory, ext, fileList);
        }
    }
    
    void FileSystem::UnmountStorageAreas()
    {
        R2_CHECK(mStorageAreas != nullptr, "We should have a non-null mStorageAreas");
        if (mStorageAreas != nullptr)
        {
            r2::sarr::Clear(*mStorageAreas);
        }
    }
    
    FileStorageArea* FileSystem::FindStorageArea(const char* path)
    {
        R2_CHECK(path != nullptr, "Got a null path!");
        if (!path)
        {
            return nullptr;
        }
        
        u64 numStorageAreas = r2::sarr::Size(*mStorageAreas);
        FileStorageArea* bestMatch = nullptr;
        
        u32 curMaxMatches = 0;

        for (u64 i = 0; i < numStorageAreas; ++i)
        {
            //Promise you won't modify me
            u32 numSubMatches = utils::NumMatchingSubPaths(path, r2::sarr::At(*mStorageAreas, i)->RootPath(), utils::PATH_SEPARATOR);
           
            if (numSubMatches > curMaxMatches)
            {
                bestMatch = r2::sarr::At(*mStorageAreas, i);
            }
        }
        
        return bestMatch;
    }
}
