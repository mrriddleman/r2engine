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
    namespace fs
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

        class File
        {
        public:
            virtual ~File() {};
            
            virtual u64 Read(void* buffer, u64 length) = 0;
            virtual u64 Write(const void* buffer, u64 length) = 0;
            
            virtual bool ReadAll(void* buffer) = 0;
            
            virtual void Seek(u64 position) = 0;
            virtual void SeekToEnd(void) = 0;
            virtual void Skip(u64 bytes) = 0;
            virtual s64 Tell(void) const = 0;
            
            virtual bool IsOpen() const = 0;
            virtual s64 Size() const = 0;
        };
    }
}

#endif /* File_h */
