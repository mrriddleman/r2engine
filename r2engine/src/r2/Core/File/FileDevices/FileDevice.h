//
//  FileDevice.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-07.
//

#ifndef FileDevice_h
#define FileDevice_h

#include "r2/Core/File/FileTypes.h"
#include "r2/Core/Memory/Allocators/LinearAllocator.h"
#include "r2/Core/Memory/Allocators/PoolAllocator.h"


namespace r2
{
    namespace fs
    {
        class File;

        class R2_API FileStorageDevice
        {
        public:
            
            FileStorageDevice(DeviceStorage storageType): mStorageType(storageType), moptrFilePool(nullptr){};
            
            virtual ~FileStorageDevice(){}
            virtual File* Open(const char* path, FileMode mode) = 0;
            virtual void Close(File* file) = 0;
            virtual bool Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles) = 0;
            virtual bool Unmount(r2::mem::LinearArena& permanentStorage) = 0;
            
            DeviceStorage GetStorageType() const {return mStorageType;}
        protected:
            //@TODO(Serge): make this into a checking pool
            r2::mem::NoCheckPoolArena* moptrFilePool;
        private:
            DeviceStorage mStorageType;
        };
        
        class R2_API FileDeviceModifier: public FileStorageDevice
        {
        public:
            
            FileDeviceModifier(DeviceModifier mod):FileStorageDevice(DeviceStorage::None), mMod(mod), mnoptrFile(nullptr){}
            
            virtual ~FileDeviceModifier(){}
            virtual File* Open(const char* path, FileMode mode) override {return nullptr;}
            virtual File* Open(File* file) = 0;
            virtual void Close(File* file) override = 0;
            virtual bool Mount(r2::mem::LinearArena& permanentStorage, u64 numFiles) override = 0;
            virtual bool Unmount(r2::mem::LinearArena& permanentStorage) override = 0;
            
            DeviceModifier GetModifier() const {return mMod;}
        protected:
            File* mnoptrFile;
        private:
            DeviceModifier mMod;
        };
    }
}

#endif /* FileDevice_h */
