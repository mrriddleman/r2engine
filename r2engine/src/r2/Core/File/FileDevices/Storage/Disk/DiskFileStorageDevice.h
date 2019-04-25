//
//  DiskFileStorageDevice.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//

#ifndef DiskFileStorageDevice_h
#define DiskFileStorageDevice_h

#include "r2/Core/File/FileDevices/FileDevice.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"

namespace r2::fs
{
    class R2_API DiskFileStorageDevice: public FileStorageDevice
    {
    public:
        //Maybe pass in a pool of DiskFiles or make them using an allocator or something?
        DiskFileStorageDevice();
        ~DiskFileStorageDevice();
        virtual bool Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles) override;
        virtual bool Unmount(r2::mem::LinearArena& permanentStorage) override;
        virtual File* Open(const char* path, FileMode mode) override;
        virtual void Close(File* file) override;
    };
}

#endif /* DiskFileStorageDevice_h */
