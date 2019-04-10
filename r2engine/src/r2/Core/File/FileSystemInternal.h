//
//  FileSystemInternal.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-07.
//

#ifndef FileSystemInternal_h
#define FileSystemInternal_h

#include "r2/Core/File/File.h"
#include "r2/Core/File/FileDevice.h"

namespace r2
{
    namespace fs
    {
        class FileStorageArea;
        
        class R2_API FileSystemInternal
        {
        public:

            FileSystemInternal(FileStorageArea& fsArea);
            ~FileSystemInternal();

            File* Open(const DeviceConfig& config, const char* path, FileMode mode);
            void Close(File* file);
            
            bool FileExists(const char* path) const;
            bool DirExists(const char* path) const;
            
            bool CreateDirectory(const char* path, bool recursive);
        private:
            
            bool Mount(FileStorageDevice* noptrDevice);
            bool Unmount(FileStorageDevice* noptrDevice);
            
            bool Mount(FileDeviceModifier* noptrDeviceMod);
            bool Unmount(FileDeviceModifier* noptrDeviceMod);
            
            void UnmountAllFileDevices();
            
            static const u8 NUM_FILE_DEVICES = u8(DeviceStorage::Count);
            static const u8 NUM_FILE_DEVICE_MODIFIERS = u8(DeviceStorage::Count);
            
            FileStorageArea& mFSArea;
            FileStorageDevice* mFileDevice;
            FileDeviceModifier* mDeviceModifiersList[NUM_FILE_DEVICE_MODIFIERS];
        };
    }
}

#endif /* FileSystemInternal_h */
