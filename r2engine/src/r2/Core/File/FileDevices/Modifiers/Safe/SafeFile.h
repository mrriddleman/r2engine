//
//  SafeFile.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-22.
//

#ifndef SafeFile_h
#define SafeFile_h

#include "r2/Core/File/File.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileStorageDevice.h"

namespace r2::fs
{
    class R2_API SafeFile final: public File
    {
    public:
        SafeFile(DiskFileStorageDevice& storageDevice);
        ~SafeFile();
        bool Open(File* noptrFile);
        bool Close();
        
        virtual u64 Read(void* buffer, u64 length) override;
        virtual u64 Read(void* buffer, u64 offset, u64 length) override;
        virtual u64 Write(const void* buffer, u64 length) override;
        
        virtual bool ReadAll(void* buffer) override;
        
        virtual void Seek(u64 position) override;
        virtual void SeekToEnd(void) override;
        virtual void Skip(u64 bytes) override;
        virtual s64 Tell(void) const override;
        
        virtual bool IsOpen() const override;
        virtual s64 Size() const override;
    private:
        static const u8 NUM_CHARS_FOR_HASH = 64+1;
        File* mnoptrFile;
        DiskFileStorageDevice& mStorageDevice;
    };
}

#endif /* SafeFile_h */
