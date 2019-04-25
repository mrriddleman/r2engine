//
//  ZipFile.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-19.
//

#ifndef ZipFile_h
#define ZipFile_h

#include "r2/Core/File/File.h"
#include "miniz.h"



namespace r2::fs
{
    class R2_API ZipFile final: public File
    {
    public:

        ZipFile();
        ~ZipFile();
        bool Open(File* noptrFile);
        bool Close();
        
        //These will uncompress the archive and read length bytes into buffer - starts at current offset
        virtual u64 Read(void* buffer, u64 length) override;
        //Same as above but starting at a byte offset
        virtual u64 Read(void* buffer, u64 offset, u64 length) override;
    
        virtual u64 Write(const void* buffer, u64 length) override;
        
        virtual bool ReadAll(void* buffer) override;
        
        virtual void Seek(u64 position) override;
        virtual void SeekToEnd(void) override;
        virtual void Skip(u64 bytes) override;
        virtual s64 Tell(void) const override;
        
        virtual bool IsOpen() const override;
        
        //The compressed archive size
        virtual s64 Size() const override;
        
        //Zip file specific
        u64 UncompressedSize() const;
        void AddMemToArchive(const void* buf, u64 len);
        u64 GetNumberOfFiles() const;
        
        u64 GetFileSizeInArchive(u32 fileIndex);
        u64 GetFileSizeInArchive(const char* filename);
        
        u32 GetFilename(u32 fileIndex, char* filename, u32 filenameBufSize);
        
        bool LocateFile(const char* filename, u32& fileIndex);
        
        bool ReadFileData(void* buffer, u32 fileIndex);
        bool ReadFileData(void* buffer, const char* filename);
        
    private:
        File* mnoptrFile;
        static const u32 MAX_DIRS = 1024;
        
        struct ZipState
        {
            u8 centralDir[MAX_DIRS];
            u32 centralDirOffsets[MAX_DIRS];
        };
        
        struct ZipArchive
        {
            u64 numFiles = 0;
            u64 centralDirectoryOffset = 0;
            u64 archiveSize = 0;

            ZipState state;
        };
        
        ZipArchive mArchive;
        
        bool InitReadArchive();
        void EndArchive();
        bool ReadCentralDir();
    };
}

#endif /* ZipFile_h */
