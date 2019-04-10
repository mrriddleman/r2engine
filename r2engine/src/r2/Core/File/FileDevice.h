//
//  FileDevice.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-07.
//

#ifndef FileDevice_h
#define FileDevice_h

#include "r2/Core/File/FileTypes.h"

namespace r2
{
    namespace fs
    {
        class File;

        class R2_API FileStorageDevice
        {
        public:
            
            FileStorageDevice(DeviceStorage storageType): mStorageType(storageType){};
            
            virtual ~FileStorageDevice(){}
            virtual File* Open(const char* path, FileMode mode) = 0;
            virtual void Close(File* file) = 0;
            
        private:
            DeviceStorage mStorageType;
        };
        
        class R2_API FileDeviceModifier
        {
        public:
            
            FileDeviceModifier(DeviceModifier mod):mMod(mod){}
            
            virtual ~FileDeviceModifier(){}
            virtual File* Open(File* file) = 0;
            virtual void Close(File* file) = 0;
        
        private:
            DeviceModifier mMod;
        };
    }
}

#endif /* FileDevice_h */
