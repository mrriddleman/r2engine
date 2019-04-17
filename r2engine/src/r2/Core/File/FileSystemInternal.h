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

            bool FileExists(const char* path) const;
            bool DirExists(const char* path) const;
            
            bool CreateDirectory(const char* path, bool recursive);
        private:
            FileStorageArea& mFSArea;
            
            FileDeviceModifier* GetModifier(DeviceModifier modType);
            
        };
    }
}

#endif /* FileSystemInternal_h */
