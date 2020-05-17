//
//  SDL2File.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//

#include "r2pch.h"
#if defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"
#include <SDL2/SDL.h>
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileUtils.h"
#include <stdio.h>
namespace r2
{
    namespace fs
    {
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
            utils::GetStringFileMode(strFileMode, mode);

            mHandle = SDL_RWFromFile(path, strFileMode);

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
            
            if (IsOpen() && buffer != nullptr && length > 0 && static_cast<s64>(offset + length) <= Size())
            {
                Seek(offset);
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
        
        //FILE* DiskFile::GetFP()
        //{
        //    R2_CHECK(IsOpen(), "The file should be open");
        //    SDL_RWops* ops = (SDL_RWops*)mHandle;
        //    return ops->hidden.stdio.fp;
        //}
        
        
    }
}

#endif
