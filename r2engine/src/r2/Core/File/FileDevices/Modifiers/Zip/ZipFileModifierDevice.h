//
//  ZipFileModifierDevice.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-19.
//

#ifndef ZipFileModifierDevice_h
#define ZipFileModifierDevice_h

#include "r2/Core/File/FileDevices/FileDevice.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"

namespace r2::fs
{
    class R2_API ZipFileModifierDevice: public FileDeviceModifier
    {
    public:
        ZipFileModifierDevice();
        ~ZipFileModifierDevice();
        virtual bool Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles) override;
        virtual bool Unmount(r2::mem::LinearArena& permanentStorage) override;
        virtual File* Open(File* file) override;
        virtual void Close(File* file) override;

    };
}

#endif /* ZipFileModifierDevice_h */
