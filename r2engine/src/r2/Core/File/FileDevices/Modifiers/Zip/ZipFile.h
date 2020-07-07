//
//  ZipFile.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-19.
//

#ifndef ZipFile_h
#define ZipFile_h

#include "r2/Core/File/File.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SHashMap.h"
#include <functional>

namespace r2::fs
{

    
    template<typename T>
    struct ZipArray
    {
        T* data = nullptr;
        u32 size = 0;
    };
    
    class R2_API ZipFile final: public File
    {
    public:

        struct ZipFileInfo
        {
            u32 fileIndex;
            u64 cetralDirOffset;
            
            u16 versionMadeBy;
            u16 versionNeeded;
            u16 bitFlag;
            u16 method;
            
            /* CRC-32 of uncompressed data. */
            u32 CRC32;
            
            /* File's compressed size. */
            u64 compressedSize;
            
            /* File's uncompressed size. Note, I've seen some old archives where directory entries had 512 bytes for their uncompressed sizes, but when you try to unpack them you actually get 0 bytes. */
            u64 uncompressedSize;
            
            /* Zip internal and external file attributes. */
            u16 internalAttrib;
            u32 externalAttrib;
            
            /* Entry's local header file offset in bytes. */
            u64 localHeaderOffset;
            
            /* Size of comment in bytes. */
            u32 commentSize;
            
            /* MZ_TRUE if the entry appears to be a directory. */
            bool isDirectory;
            
            /* MZ_TRUE if the entry uses encryption/strong encryption (which miniz_zip doesn't support) */
            bool isEncrypted;
            
            /* MZ_TRUE if the file is not encrypted, a patch file, and if it uses a compression method we support. */
            bool isSupported;
            
            /* Filename. If string ends in '/' it's a subdirectory entry. */
            /* Guaranteed to be zero terminated, may be truncated to fit. */
            char filename[r2::fs::FILE_PATH_LENGTH];
            
            /* Comment field. */
            /* Guaranteed to be zero terminated, may be truncated to fit. */
            char comment[r2::fs::FILE_PATH_LENGTH];
        };
        
        ZipFile();
        ~ZipFile();
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
        
        //The compressed archive size
        virtual s64 Size() const override;
        
        //Zip file specific
        bool InitArchive(r2::mem::AllocateFunc alloc, r2::mem::FreeFunc free);
        u64 TotalUncompressedSize() const;
        u32 CentralDirSize() const;
        u64 GetNumberOfFiles() const;
        
        u32 GetFilename(u32 fileIndex, char* filename, u32 filenameBufSize);
        
        bool IsFileADirectory(u32 fileIndex) const;
        bool FindFile(const char* filename, u32& fileIndex, bool ignorePath = false);
        bool GetFileInfo(u32 fileIndex, ZipFileInfo& info, bool* foundExtraData) const;
        bool FindFile(u64 hash, u32& fileIndex);

        bool ReadUncompressedFileData(void* buffer, u64 bufferSize, u32 fileIndex);
        bool ReadUncompressedFileData(void* buffer, u64 bufferSize, const char* filename);
        bool ReadUncompressedFileDataByHash(void* buffer, u64 bufferSize, u64 hash);

    private:
        
        bool InitReadArchive();
        void EndArchive();
        bool ReadCentralDir();
        bool ZipReaderLocateHeaderSig(u32, u32, u64&);
        
        File* mnoptrFile;

        struct ZipState
        {
            ZipArray<byte> centralDir;
            ZipArray<u32> centralDirOffsets;
            u64 fileArchiveStartOffset = 0;
            b32 isZip64 = false;
            b32 zip64HasExtendedInfoFields = false;
        };
        
        struct ZipArchive
        {
            u64 totalFiles = 0;
            u64 centralDirectoryOffset = 0;
            u64 archiveSize = 0;
        
            ZipState state;
        };
        
        ZipArchive mArchive;
        r2::mem::AllocateFunc mAlloc;
        r2::mem::FreeFunc mFree;
        r2::SHashMap<u32>* mFileIndexLookup;
    };
}

#endif /* ZipFile_h */
