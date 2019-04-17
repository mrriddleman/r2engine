//
//  File.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//

#ifndef File_h
#define File_h

#include "r2/Core/File/FileTypes.h"

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
        private:
            FileStorageDevice* mDevice;
        };
    }
}

#endif /* File_h */
