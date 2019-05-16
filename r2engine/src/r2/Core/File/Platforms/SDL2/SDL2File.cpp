//
//  SDL2File.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//


#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"
#include <SDL2/SDL.h>

namespace r2
{
    namespace fs
    {
        void GetStringFileMode(char buf[4], FileMode mode)
        {
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
        }

        DiskFile::DiskFile():mHandle(nullptr)
        {
        }
        
        DiskFile::~DiskFile()
        {
            if (IsOpen())
            {
                Close();
            }
        }

        bool DiskFile::Open(const char* path, FileMode mode)
        {
            R2_CHECK(path != nullptr, "Null Path!");
            R2_CHECK(path != "", "Empty path!");
            
            R2_CHECK(mHandle == nullptr, "We shouldn't have any handle yet?");
            if (IsOpen())
            {
                return true;
            }
            
            char strFileMode[4];
            GetStringFileMode(strFileMode, mode);

            mHandle = SDL_RWFromFile(path, strFileMode);
            //R2_CHECK(mHandle != nullptr, "Handle should not be nullptr!");
            if (mHandle)
            {
                SetFilePath(path);
                SetFileMode(mode);
            }
            
            return mHandle != nullptr;
        }
        
        void DiskFile::Close()
        {
            R2_CHECK(IsOpen(), "Trying to close a closed file?");
            SDL_RWclose((SDL_RWops*)mHandle);
            
            mHandle = nullptr;
            SetFileDevice(nullptr);
            
        }
        
        u64 DiskFile::Read(void* buffer, u64 length)
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
        
        u64 DiskFile::Read(void* buffer, u64 offset, u64 length)
        {
            R2_CHECK(IsOpen(), "The file isn't open?");
            R2_CHECK(buffer != nullptr, "The buffer is null?");
            R2_CHECK(length > 0, "You should want to read more than 0 bytes!");
            
            if (IsOpen() && buffer != nullptr && length > 0 && offset < length)
            {
                Seek(Tell() + offset);
                return Read(buffer, length);
            }
            
            return 0;
        }
        
        u64 DiskFile::Write(const void* buffer, u64 length)
        {
            R2_CHECK(IsOpen(), "The file isn't open?");
            R2_CHECK(buffer != nullptr, "nullptr buffer?");
            R2_CHECK(length > 0, "The length is 0?");
            
            if (IsOpen() && buffer != nullptr && length > 0)
            {
                u64 numBytesWritten = SDL_RWwrite((SDL_RWops*)mHandle, buffer, sizeof(byte), length);
                R2_CHECK(numBytesWritten == length, "We couldn't write out all %llu bytes\n", length);
                return numBytesWritten;
            }
            
            return 0;
        }
        
        bool DiskFile::ReadAll(void* buffer)
        {
            R2_CHECK(IsOpen(), "The file isn't open?");
            R2_CHECK(buffer != nullptr, "nullptr buffer?");
            
            Seek(0);
            const u64 size = Size();
            u64 bytesRead = Read(buffer, size);
            
            R2_CHECK(bytesRead == size, "We should have read the whole file!");
            return bytesRead == size;
        }
        
        void DiskFile::Seek(u64 position)
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWseek((SDL_RWops*)mHandle, position, RW_SEEK_SET);
        }
        
        void DiskFile::SeekToEnd(void)
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWseek((SDL_RWops*)mHandle, 0, RW_SEEK_END);
        }
        
        void DiskFile::Skip(u64 bytes)
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWseek((SDL_RWops*)mHandle, bytes, RW_SEEK_CUR);
        }
        
        s64 DiskFile::Tell(void) const
        {
            R2_CHECK(IsOpen(), "The file should be open");
            return SDL_RWtell((SDL_RWops*)mHandle);
        }
        
        bool DiskFile::IsOpen() const
        {
            return mHandle != nullptr;
        }
        
        s64 DiskFile::Size() const
        {
            R2_CHECK(IsOpen(), "The file should be open");
            return SDL_RWsize((SDL_RWops*)mHandle);
        }
        
        FILE* DiskFile::GetFP()
        {
            R2_CHECK(IsOpen(), "The file should be open");
            SDL_RWops* ops = (SDL_RWops*)mHandle;
            return ops->hidden.stdio.fp;
        }
        
        
    }
}

#endif