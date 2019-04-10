//
//  DiskFileStorageDevice.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-09.
//

#include "DiskFileStorageDevice.h"

namespace r2::fs
{
    DiskFileStorageDevice::DiskFileStorageDevice()
        :FileStorageDevice(DeviceStorage::Disk)
    {
        
    }
    
    File* DiskFileStorageDevice::Open(const char* path, FileMode mode)
    {
        return nullptr;
    }
    
    void DiskFileStorageDevice::Close(File* file)
    {
        
    }
}
