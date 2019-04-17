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
            DeviceStorage GetStorageType() const {return mStorageType;}
        private:
            DeviceStorage mStorageType;
        };
        
        class R2_API FileDeviceModifier: public FileStorageDevice
        {
        public:
            
            FileDeviceModifier(DeviceModifier mod):FileStorageDevice(DeviceStorage::None), mMod(mod){}
            
            virtual ~FileDeviceModifier(){}
            virtual File* Open(const char* path, FileMode mode) override {return nullptr;}
            virtual File* Open(File* file) = 0;
            virtual void Close(File* file) override = 0;
            
            DeviceModifier GetModifier() const {return mMod;}
        protected:
            File* mnoptrFile;
        private:
            DeviceModifier mMod;
            
        };
    }
}

#endif /* FileDevice_h */
