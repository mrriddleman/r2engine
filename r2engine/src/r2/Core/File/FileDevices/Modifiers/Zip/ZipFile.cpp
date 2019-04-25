//
//  ZipFile.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-19.
//


//Based on https://github.com/richgel999/miniz by Rich Geldreich
//Mostly ripped but rejigged to fit this API better

#include "ZipFile.h"
#include "r2/Core/File/FileDevices/FileDevice.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"

namespace r2::fs
{
    const u32 ZipFile::MAX_DIRS;
    
    ZipFile::ZipFile(): mnoptrFile(nullptr)
    {
        
    }
    
    ZipFile::~ZipFile()
    {
        
    }
    
    bool ZipFile::Open(File* noptrFile)
    {
        R2_CHECK(noptrFile != nullptr, "File cannot be nullptr");
        mnoptrFile = noptrFile;
        return true;
    }
    
    bool ZipFile::Close()
    {
        auto* fileDevice = mnoptrFile->GetFileDevice();
        fileDevice->Close(mnoptrFile);
        return true;
    }
    
    u64 ZipFile::Read(void* buffer, u64 length)
    {
        
        //read length bytes from an archive (compressed) file
        return 0;
    }
    
    u64 ZipFile::Read(void* buffer, u64 offset, u64 length)
    {
        return 0;
    }
    
    u64 ZipFile::Write(const void* buffer, u64 length)
    {
        //write length bytes into an archive
        return 0;
    }
    
    bool ZipFile::ReadAll(void* buffer)
    {
        //read all bytes from an archive (compressed) file
        return false;
    }
    
    void ZipFile::Seek(u64 position)
    {
        
    }
    
    void ZipFile::SeekToEnd(void)
    {
        
    }
    
    void ZipFile::Skip(u64 bytes)
    {
        
    }
    
    s64 ZipFile::Tell(void) const
    {
        return 0;
    }
    
    bool ZipFile::IsOpen() const
    {
        R2_CHECK(mnoptrFile != nullptr, "We must have a valid mnoptrFile!");
        return mnoptrFile && mnoptrFile->IsOpen();
    }
    
    s64 ZipFile::Size() const
    {
        return mnoptrFile->Size();
    }
    
    
    //Zip file specific
    u64 ZipFile::UncompressedSize() const
    {
        return 0;
    }
    
    void ZipFile::AddMemToArchive(const void* buf, u64 len)
    {
        
    }
    
    u64 ZipFile::GetNumberOfFiles() const
    {
        return 0;
    }
    
    u64 ZipFile::GetFileSizeInArchive(u32 fileIndex)
    {
        return 0;
    }
    
    u64 ZipFile::GetFileSizeInArchive(const char* filename)
    {
        return 0;
    }
    
    u32 ZipFile::GetFilename(u32 fileIndex, char* filename, u32 filenameBufSize)
    {
        return 0;
    }
    
    bool ZipFile::LocateFile(const char* filename, u32& fileIndex)
    {
        return false;
    }
    
    bool ZipFile::ReadFileData(void* buffer, u32 fileIndex)
    {
        return false;
    }
    
    bool ZipFile::ReadFileData(void* buffer, const char* filename)
    {
        u32 fileIndex;
        if(!LocateFile(filename, fileIndex))
        {
            return false;
        }

        return ReadFileData(buffer, fileIndex);
    }
    
    bool ZipFile::InitReadArchive()
    {
        memset(&mArchive, 0, sizeof(ZipArchive));
        
        mArchive.archiveSize = Size();
        return ReadCentralDir();
    }
    
    bool ZipFile::ReadCentralDir()
    {
        return false;
    }
}
