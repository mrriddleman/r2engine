//
//  SafeFileModifierDevice.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-22.
//

#ifndef SafeFileModifierDevice_h
#define SafeFileModifierDevice_h

#include "r2/Core/File/FileDevices/FileDevice.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileStorageDevice.h"

namespace r2::fs
{
    class R2_API SafeFileModifierDevice final: public FileDeviceModifier
    {
    public:
        SafeFileModifierDevice(DiskFileStorageDevice& storageDevice);
        ~SafeFileModifierDevice();
        virtual bool Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles) override;
        virtual bool Unmount(r2::mem::LinearArena& permanentStorage) override;
        virtual File* Open(File* file) override;
        virtual void Close(File* file) override;
        
    private:
        DiskFileStorageDevice& mStorageDevice;
    };
}

#endif /* SafeFileModifierDevice_h */
