//
//  SDL2File.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//


#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Core/File/File.h"
#include <SDL2/SDL.h>

namespace r2
{
    namespace file
    {
        
        const char* GetStringFileMode(FileMode mode)
        {
            static char buf[4];
            u8 size = 0;
            
            if (mode.IsSet(Mode::Read) &&
                !mode.IsSet(Mode::Append) &&
                !mode.IsSet(Mode::Write) &&
                !mode.IsSet(Recreate))
            {
                buf[size++] = 'r';
            }
            else if ((mode.IsSet(Mode::Read) && mode.IsSet(Mode::Write)) && !mode.IsSet(Mode::Recreate) && !mode.IsSet(Mode::Append))
            {
                buf[size++] = 'r';
                buf[size++] = '+';
            }
            else if (mode.IsSet(Mode::Write) && !mode.IsSet(Mode::Read) && !mode.IsSet(Mode::Append))
            {
                buf[size++] = 'w';
            }
            else if (mode.IsSet(Mode::Write) && mode.IsSet(Mode::Read) && !mode.IsSet(Mode::Append))
            {
                buf[size++] = 'w';
                buf[size++] = '+';
            }
            else if (mode.IsSet(Mode::Append) && !mode.IsSet(Mode::Read))
            {
                buf[size++] = 'a';
            }
            else if (mode.IsSet(Mode::Append) && mode.IsSet(Mode::Read))
            {
                buf[size++] = 'a';
                buf[size++] = '+';
            }
            else
            {
                R2_CHECK(false, "Unknown FileMode combination!");
            }
            
            //Always add at the end
            if (mode.IsSet(Mode::Binary))
            {
                buf[size++] = 'b';
            }
            
            buf[size] = '\0';
            
            return buf;
        }

        File::File():mHandle(nullptr)
        {
        }
        
        File::~File()
        {
            if (IsOpen())
            {
                Close();
            }
        }

        bool File::Open(const char* path, FileMode mode)
        {
            mHandle = SDL_RWFromFile(path, GetStringFileMode(mode));
            return mHandle != nullptr;
        }
        
        void File::Close()
        {
            R2_CHECK(IsOpen(), "Trying to close a closed file?");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            SDL_RWclose(ops);
            
            mHandle = nullptr;
        }
        
        u64 File::Read(void* buffer, u64 length)
        {
            R2_CHECK(IsOpen(), "The file isn't open?");
            R2_CHECK(buffer != nullptr, "The buffer is null?");
            R2_CHECK(length > 0, "You should want to read more than 0 bytes!");
            
            if (IsOpen() && buffer != nullptr && length > 0)
            {
                u64 readBytes = SDL_RWread((SDL_RWops*)mHandle, buffer, sizeof(byte), length);
                return readBytes;
            }
            
            return 0;
        }
        
        u64 File::Write(const void* buffer, u64 length)
        {
            R2_CHECK(IsOpen(), "The file isn't open?");
            R2_CHECK(buffer != nullptr, "nullptr buffer?");
            R2_CHECK(length > 0, "The length is 0?");
            
            if (IsOpen() && buffer != nullptr && length > 0)
            {
                u64 numBytesWritten = SDL_RWwrite((SDL_RWops*)mHandle, buffer, sizeof(byte), length);
                return numBytesWritten;
            }
            
            return 0;
        }
        
        bool File::ReadAll(void* buffer)
        {
            R2_CHECK(IsOpen(), "The file isn't open?");
            R2_CHECK(buffer != nullptr, "nullptr buffer?");
            
            const u64 size = Size();
            
            u64 bytesRead = Read(buffer, size);
            
            R2_CHECK(bytesRead == size, "We should have read the whole file!");
            return bytesRead == size;
        }
        
        void File::Seek(u64 position)
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            
            SDL_RWseek(ops, position, RW_SEEK_SET);
        }
        
        void File::SeekToEnd(void)
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            SDL_RWseek(ops, 0, RW_SEEK_END);
        }
        
        void File::Skip(u64 bytes)
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            SDL_RWseek(ops, bytes, RW_SEEK_CUR);
        }
        
        s64 File::Tell(void) const
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            
            return SDL_RWtell(ops);
        }
        
        bool File::IsOpen() const
        {
            return mHandle != nullptr;
        }
        
        s64 File::Size() const
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            return SDL_RWsize(ops);
        }
    }
}

#endif
