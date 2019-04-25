//
//  SDL2File.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#ifndef SDL2File_h
#define SDL2File_h

#include "r2/Core/File/File.h"

namespace r2
{
    namespace fs
    {
        class R2_API DiskFile final: public File
        {
        public:
            
            using FileHandle = void*;
            
            DiskFile();
            ~DiskFile();
            
            bool Open(const char* path, FileMode mode);
            void Close();

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
            
            FILE* GetFP();
        private:
            FileHandle mHandle;
        };
    }
}

#endif /* SDL2File_h */
