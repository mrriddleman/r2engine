//
//  FileSystem.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-15.
//

#ifndef FileSystem_h
#define FileSystem_h

#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/File/FileTypes.h"
#include "r2/Core/Containers/SArray.h"

namespace r2::fs
{
    class FileStorageArea;
    class File;
    
    class FileSystem
    {
    public:

        //Not to be used by application
        static bool Init(r2::mem::LinearArena& arena, u32 storageAreaCapacity);
        static void Mount(FileStorageArea& storageArea);
        static void Shutdown(r2::mem::LinearArena& arena);
        
        static File* Open(const DeviceConfig& config, const char* path, FileMode mode);
        static void Close(File* file);
        
        static bool FileExists(const char* path);
        static bool DeleteFile(const char* path);
        static bool RenameFile(const char* existingFile, const char* newName);
        static bool CopyFile(const char* existingFile, const char* newName);
        
        static bool DirExists(const char* path);
        static bool CreateDirectory(const char* path, bool recursive);
        static bool DeleteDirectory(const char* path);
    private:
        static void UnmountStorageAreas();
        static FileStorageArea* FindStorageArea(const char* path);
        static r2::SArray<FileStorageArea*>* mStorageAreas;
    };
}

#endif /* FileSystem_h */
