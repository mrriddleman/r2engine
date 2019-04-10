//
//  DiskFileStorageDevice.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//

#ifndef DiskFileStorageDevice_h
#define DiskFileStorageDevice_h

#include "r2/Core/File/FileDevice.h"

namespace r2::fs
{
    class DiskFileStorageDevice: public FileStorageDevice
    {
        
        //Maybe pass in a pool of DiskFiles or make them using an allocator or something?
        DiskFileStorageDevice();
        virtual File* Open(const char* path, FileMode mode) override;
        virtual void Close(File* file) override;
    };
}

#endif /* DiskFileStorageDevice_h */
