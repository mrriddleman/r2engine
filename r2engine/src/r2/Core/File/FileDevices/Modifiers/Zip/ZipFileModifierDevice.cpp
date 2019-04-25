//
//  ZipFileModifierDevice.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-19.
//

#include "ZipFileModifierDevice.h"

namespace r2::fs
{
    ZipFileModifierDevice::ZipFileModifierDevice(): FileDeviceModifier(DeviceModifier::Zip)
    {
        
    }
    
    bool ZipFileModifierDevice::Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles)
    {
        return false;
    }
    
    bool ZipFileModifierDevice::Unmount(r2::mem::LinearArena& permanentStorage)
    {
        return false;
    }
    
    File* ZipFileModifierDevice::Open(File* file)
    {
        return nullptr;
    }
    
    void ZipFileModifierDevice::Close(File* file)
    {
        
    }
}
