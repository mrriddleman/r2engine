//
//  File.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//

#ifndef File_h
#define File_h

#include "r2/Core/File/FileTypes.h"
#include <cstring>

namespace r2
{
    namespace fs
    {
        class FileStorageDevice;
        
        class R2_API File
        {
        public:
            virtual ~File() {mDevice = nullptr;}
            
            virtual u64 Read(void* buffer, u64 length) = 0;
            virtual u64 Read(void* buffer, u64 offset, u64 length) = 0;
            virtual u64 Write(const void* buffer, u64 length) = 0;
            
            virtual bool ReadAll(void* buffer) = 0;
            
            virtual void Seek(u64 position) = 0;
            virtual void SeekToEnd(void) = 0;
            virtual void Skip(u64 bytes) = 0;
            virtual s64 Tell(void) const = 0;
            
            virtual bool IsOpen() const = 0;
            virtual s64 Size() const = 0;
            
            //Not to be used by anyone except the file system!
            void SetFileDevice(FileStorageDevice* fileDevice) {mDevice = fileDevice;}
            FileStorageDevice* GetFileDevice() {return mDevice;}
            void SetFilePath(const char* filePath){strcpy(mFilename, filePath);}
            const char* GetFilePath() const {return mFilename;}
            DeviceConfig GetDeviceConfig() const {return mDeviceConfig;}
            void SetStorageDeviceConfig(DeviceStorage storage) { mDeviceConfig.SetDeviceStorage(storage);}
            void AddModifierConfig(DeviceModifier mod) {mDeviceConfig.AddModifier(mod);}
            FileMode GetFileMode() const {return mMode;}
            void SetFileMode(FileMode mode) {mMode = mode;}
            
        private:
            FileStorageDevice* mDevice;
            char mFilename[Kilobytes(1)];
            DeviceConfig mDeviceConfig;
            FileMode mMode;
        };
        
        
    }
}

#endif /* File_h */
