//
//  File.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//

#ifndef File_h
#define File_h
#include <stdio.h>

namespace r2
{
    namespace file
    {
        enum Mode
        {
            Read = 1 << 0,
            Write = 1 << 1,
            Append = 1 << 2,
            Recreate = 1 << 3,
            Binary = 1 << 4,
        };
        
        using FileMode = r2::Flags<Mode, u32>;
        
        using FileHandle = void*;
        
        class File
        {
        public:
            
            File();
            ~File();
            
            File(const File& o) = delete;
            File& operator=(const File& o) = delete;
            
            File(File && o) = delete;
            File& operator=(File && o) = delete;
            
            bool Open(const char* path, FileMode mode);
            void Close();
            u64 Read(void* buffer, u64 length);
            u64 Write(const void* buffer, u64 length);
            
            bool ReadAll(void* buffer);
            
            void Seek(u64 position);
            void SeekToEnd(void);
            void Skip(u64 bytes);
            s64 Tell(void) const;
            
            bool IsOpen() const;
            s64 Size() const;
            
        private:
            FileHandle mHandle;
        };
    }
}

#endif /* File_h */
