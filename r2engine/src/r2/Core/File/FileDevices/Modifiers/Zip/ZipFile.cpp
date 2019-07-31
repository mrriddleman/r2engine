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
#include "r2/Core/File/PathUtils.h"
#include "miniz.h"


namespace
{
    /* ZIP archive identifiers and record sizes */
    const u32 R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG = 0x06054b50;
    const u32 R2_ZIP_CENTRAL_DIR_HEADER_SIG = 0x02014b50;
    const u32 R2_ZIP_LOCAL_DIR_HEADER_SIG = 0x04034b50;
    const u32 R2_ZIP_LOCAL_DIR_HEADER_SIZE = 30;
    const u32 R2_ZIP_CENTRAL_DIR_HEADER_SIZE = 46;
    const u32 R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE = 22;
    
    /* ZIP64 archive identifier and record sizes */
    const u32 R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIG = 0x06064b50;
    const u32 R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIG = 0x07064b50;
    const u32 R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIZE = 56;
    const u32 R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIZE = 20;
    const u32 R2_ZIP64_EXTENDED_INFORMATION_FIELD_HEADER_ID = 0x0001;
    const u32 R2_ZIP_DATA_DESCRIPTOR_ID = 0x08074b50;
    const u32 R2_ZIP_DATA_DESCRIPTER_SIZE64 = 24;
    const u32 R2_ZIP_DATA_DESCRIPTER_SIZE32 = 16;
    
    /* Central directory header record offsets */
    const u32 R2_ZIP_CDH_SIG_OFS = 0;
    const u32 R2_ZIP_CDH_VERSION_MADE_BY_OFS = 4;
    const u32 R2_ZIP_CDH_VERSION_NEEDED_OFS = 6;
    const u32 R2_ZIP_CDH_BIT_FLAG_OFS = 8;
    const u32 R2_ZIP_CDH_METHOD_OFS = 10;
    const u32 R2_ZIP_CDH_FILE_TIME_OFS = 12;
    const u32 R2_ZIP_CDH_FILE_DATE_OFS = 14;
    const u32 R2_ZIP_CDH_CRC32_OFS = 16;
    const u32 R2_ZIP_CDH_COMPRESSED_SIZE_OFS = 20;
    const u32 R2_ZIP_CDH_DECOMPRESSED_SIZE_OFS = 24;
    const u32 R2_ZIP_CDH_FILENAME_LEN_OFS = 28;
    const u32 R2_ZIP_CDH_EXTRA_LEN_OFS = 30;
    const u32 R2_ZIP_CDH_COMMENT_LEN_OFS = 32;
    const u32 R2_ZIP_CDH_DISK_START_OFS = 34;
    const u32 R2_ZIP_CDH_INTERNAL_ATTR_OFS = 36;
    const u32 R2_ZIP_CDH_EXTERNAL_ATTR_OFS = 38;
    const u32 R2_ZIP_CDH_LOCAL_HEADER_OFS = 42;
    
    const u32 R2_ZIP_ECDH_SIG_OFS = 0;
    const u32 R2_ZIP_ECDH_NUM_THIS_DISK_OFS = 4;
    const u32 R2_ZIP_ECDH_NUM_DISK_CDIR_OFS = 6;
    const u32 R2_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS = 8;
    const u32 R2_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS = 10;
    const u32 R2_ZIP_ECDH_CDIR_SIZE_OFS = 12;
    const u32 R2_ZIP_ECDH_CDIR_OFS_OFS = 16;
    const u32 R2_ZIP_ECDH_COMMENT_SIZE_OFS = 20;
    
    /* Local directory header offsets */
    const u32 R2_ZIP_LDH_SIG_OFS = 0;
    const u32 R2_ZIP_LDH_VERSION_NEEDED_OFS = 4;
    const u32 R2_ZIP_LDH_BIT_FLAG_OFS = 6;
    const u32 R2_ZIP_LDH_METHOD_OFS = 8;
    const u32 R2_ZIP_LDH_FILE_TIME_OFS = 10;
    const u32 R2_ZIP_LDH_FILE_DATE_OFS = 12;
    const u32 R2_ZIP_LDH_CRC32_OFS = 14;
    const u32 R2_ZIP_LDH_COMPRESSED_SIZE_OFS = 18;
    const u32 R2_ZIP_LDH_DECOMPRESSED_SIZE_OFS = 22;
    const u32 R2_ZIP_LDH_FILENAME_LEN_OFS = 26;
    const u32 R2_ZIP_LDH_EXTRA_LEN_OFS = 28;
    const u32 R2_ZIP_LDH_BIT_FLAG_HAS_LOCATOR = 1 << 3;
    
    /* ZIP64 End of central directory locator offsets */
    const u32 R2_ZIP64_ECDL_SIG_OFS = 0;                    /* 4 bytes */
    const u32 R2_ZIP64_ECDL_NUM_DISK_CDIR_OFS = 4;          /* 4 bytes */
    const u32 R2_ZIP64_ECDL_REL_OFS_TO_ZIP64_ECDR_OFS = 8;  /* 8 bytes */
    const u32 R2_ZIP64_ECDL_TOTAL_NUMBER_OF_DISKS_OFS = 16; /* 4 bytes */
    
    /* ZIP64 End of central directory header offsets */
    const u32 R2_ZIP64_ECDH_SIG_OFS = 0;                       /* 4 bytes */
    const u32 R2_ZIP64_ECDH_SIZE_OF_RECORD_OFS = 4;            /* 8 bytes */
    const u32 R2_ZIP64_ECDH_VERSION_MADE_BY_OFS = 12;          /* 2 bytes */
    const u32 R2_ZIP64_ECDH_VERSION_NEEDED_OFS = 14;           /* 2 bytes */
    const u32 R2_ZIP64_ECDH_NUM_THIS_DISK_OFS = 16;            /* 4 bytes */
    const u32 R2_ZIP64_ECDH_NUM_DISK_CDIR_OFS = 20;            /* 4 bytes */
    const u32 R2_ZIP64_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS = 24; /* 8 bytes */
    const u32 R2_ZIP64_ECDH_CDIR_TOTAL_ENTRIES_OFS = 32;       /* 8 bytes */
    const u32 R2_ZIP64_ECDH_CDIR_SIZE_OFS = 40;                /* 8 bytes */
    const u32 R2_ZIP64_ECDH_CDIR_OFS_OFS = 48;                 /* 8 bytes */
    const u32 R2_ZIP_VERSION_MADE_BY_DOS_FILESYSTEM_ID = 0;
    const u32 R2_ZIP_DOS_DIR_ATTRIBUTE_BITFLAG = 0x10;
    const u32 R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_IS_ENCRYPTED = 1;
    const u32 R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_COMPRESSED_PATCH_FLAG = 32;
    const u32 R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_USES_STRONG_ENCRYPTION = 64;
    const u32 R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_LOCAL_DIR_IS_MASKED = 8192;
    const u32 R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_UTF8 = 1 << 11;
    
    template<typename T>
    bool AllocateZipArray(r2::mem::AllocateFunc allocFunc, r2::fs::ZipArray<T>& zipArray, u32 capacity, u32 alignment)
    {
        R2_CHECK(zipArray.data == nullptr, "We should have an empty array");
        
        zipArray.data = (T*)allocFunc(capacity * sizeof(T), alignment);
        
        zipArray.size = capacity;
        
        return zipArray.data != nullptr;
    }
    
    template<typename T>
    void FreeZipArray(r2::mem::FreeFunc freeFunc, r2::fs::ZipArray<T>& zipArray)
    {
        R2_CHECK(zipArray.data != nullptr, "We shouldn't have an empty array");
        freeFunc((byte*)zipArray.data);
        
        zipArray.data = nullptr;
        zipArray.size = 0;
    }
    
    
    
    bool zipStringEqual(const char *pA, const char *pB, u32 len, bool caseSensitive)
    {
        mz_uint i;
        if (caseSensitive)
            return 0 == memcmp(pA, pB, len);
        for (i = 0; i < len; ++i)
            if (tolower(pA[i]) != tolower(pB[i]))
                return false;
        return true;
    }
}
#define ZIP_FILE_INITIALIZED (mAlloc != nullptr && mFree != nullptr)

namespace r2::fs
{
    ZipFile::ZipFile(): mnoptrFile(nullptr), mAlloc(nullptr), mFree(nullptr)
    {
        
    }
    
    ZipFile::~ZipFile()
    {
        
    }
    
    bool ZipFile::Open(File* noptrFile)
    {
        R2_CHECK(noptrFile != nullptr, "File cannot be nullptr");
        if (!noptrFile)
        {
            return false;
        }
        
        mnoptrFile = noptrFile;
        
        return true;
    }
    
    bool ZipFile::Close()
    {
        EndArchive();
        return true;
    }
    
    u64 ZipFile::Read(void* buffer, u64 length)
    {
        //read length bytes from an archive (compressed) file
        return mnoptrFile->Read(buffer, length);
    }
    
    u64 ZipFile::Read(void* buffer, u64 offset, u64 length)
    {
        return mnoptrFile->Read(buffer, offset, length);
    }
    
    u64 ZipFile::Write(const void* buffer, u64 length)
    {
        //write length bytes into an archive
        R2_CHECK(false, "Can't write to a ZipFile - read only");
        return 0;
    }
    
    bool ZipFile::ReadAll(void* buffer)
    {
        //read all bytes from an archive (compressed) file
        return mnoptrFile->ReadAll(buffer);
    }
    
    void ZipFile::Seek(u64 position)
    {
        mnoptrFile->Seek(position);
    }
    
    void ZipFile::SeekToEnd(void)
    {
        mnoptrFile->SeekToEnd();
    }
    
    void ZipFile::Skip(u64 bytes)
    {
        mnoptrFile->Skip(bytes);
    }
    
    s64 ZipFile::Tell(void) const
    {
        return mnoptrFile->Tell();
    }
    
    bool ZipFile::IsOpen() const
    {
        R2_CHECK(mnoptrFile != nullptr, "We must have a valid mnoptrFile!");
        return mnoptrFile &&
        mnoptrFile->IsOpen() &&
        mArchive.state.centralDir.data != nullptr &&
        mArchive.state.centralDirOffsets.data != nullptr;
    }
    
    s64 ZipFile::Size() const
    {
        return mnoptrFile->Size();
    }
    
    //Zip file specific
    bool ZipFile::InitArchive(r2::mem::AllocateFunc alloc, r2::mem::FreeFunc free)
    {
        R2_CHECK(alloc != nullptr, "We must have an allocation function for zip files!");
        if (!alloc)
        {
            return false;
        }
        mAlloc = alloc;
        
        R2_CHECK(free != nullptr, "We must have a free function for zip files!");
        if (!free)
        {
            return false;
        }
        mFree = free;
        
        if (mnoptrFile->Size() < R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
        {
            bool result = Close();
            R2_CHECK(result, "File couldn't be closed");
            return false;
        }
        
        if (!InitReadArchive())
        {
            Close();
            R2_CHECK(false, "Failed to initialize archive reader");
            return false;
        }
        
        if (!ReadCentralDir()) {
            Close();
            R2_CHECK(false, "Couldn't read cental directory");
            return false;
        }
        
        return true;
    }
    
    
    u64 ZipFile::TotalUncompressedSize() const
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return 0;
        }
        u64 totalUncompressedSize = 0;
        
        for (u64 i = 0; i < mArchive.totalFiles; ++i)
        {
            ZipFile::ZipFileInfo info;
            bool gotInfo = GetFileInfo(i, info, nullptr);
            
            R2_CHECK(gotInfo, "We somehow didn't get the file info for file index: %llu", i);
            
            totalUncompressedSize += info.uncompressedSize;
        }
    
        return totalUncompressedSize;
    }
    
    u32 ZipFile::CentralDirSize() const
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return 0;
        }
        
        return mArchive.state.centralDir.size;
    }
    
    u64 ZipFile::GetNumberOfFiles() const
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return 0;
        }
        
        return mArchive.totalFiles;
    }
    
    u32 ZipFile::GetFilename(u32 fileIndex, char* filename, u32 filenameBufSize)
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return 0;
        }
        
        
        u32 n;
        const u8 *p = &mArchive.state.centralDir.data[mArchive.state.centralDirOffsets.data[fileIndex]];
        if (!p)
        {
            if (filenameBufSize)
                filename[0] = '\0';
            R2_CHECK(false, "");
            return 0;
        }
        n = MZ_READ_LE16(p + R2_ZIP_CDH_FILENAME_LEN_OFS);
        if (filenameBufSize)
        {
            n = MZ_MIN(n, filenameBufSize - 1);
            memcpy(filename, p + R2_ZIP_CENTRAL_DIR_HEADER_SIZE, n);
            filename[n] = '\0';
        }
        return n + 1;
    }
    
    bool ZipFile::IsFileADirectory(u32 fileIndex) const
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        u32 filenameLen, attributeMappingID, externalAttrib;
        const u8 *p = &mArchive.state.centralDir.data[mArchive.state.centralDirOffsets.data[fileIndex]];
        if (!p)
        {
            R2_CHECK(false, "Invalid central directory entry");
            return false;
        }
        
        filenameLen = MZ_READ_LE16(p + R2_ZIP_CDH_FILENAME_LEN_OFS);
        if (filenameLen)
        {
            if (*(p + R2_ZIP_CENTRAL_DIR_HEADER_SIZE + filenameLen-1) == r2::fs::utils::PATH_SEPARATOR)
            {
                return true;
            }
        }
        
        attributeMappingID = MZ_READ_LE16(p + R2_ZIP_CDH_VERSION_MADE_BY_OFS) >> 8;
        (void)attributeMappingID;
        
        externalAttrib = MZ_READ_LE32(p + R2_ZIP_CDH_EXTERNAL_ATTR_OFS);
        if ((externalAttrib & R2_ZIP_DOS_DIR_ATTRIBUTE_BITFLAG) != 0)
        {
            return true;
        }
        
        return false;
    }
    
    bool ZipFile::FindFile(const char* filename, u32& fileIndex, bool ignorePath)
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        u32 tempFileIndex;
        u64 nameLength;// commentLength;
        
        if (!filename)
        {
            R2_CHECK(false, "File name is null!");
            return false;
        }
        
        //@TODO(Serge): binary search if sorted
        
        nameLength = strlen(filename);
        if (nameLength > MZ_UINT16_MAX)
        {
            R2_CHECK(false, "Invalid parameter");
            return false;
        }
        
        //NOTE: removed comment code here - maybe needed?
        
        for (tempFileIndex = 0; tempFileIndex < mArchive.totalFiles; ++tempFileIndex)
        {
            const u8* header = &mArchive.state.centralDir.data[ mArchive.state.centralDirOffsets.data[tempFileIndex]];
            
            u32 filenameLength = MZ_READ_LE16(header + R2_ZIP_CDH_FILENAME_LEN_OFS);
            const char* zipFileName = (const char*)header + R2_ZIP_CENTRAL_DIR_HEADER_SIZE;
            if (filenameLength < nameLength)
            {
                continue;
            }
            
            //NOTE: removed comment code here - maybe needed?
            
            if (ignorePath && filenameLength)
            {
                int ofs = filenameLength - 1;
                do
                {
                    if ((filename[ofs] == '/') || (filename[ofs] == '\\') || (filename[ofs] == ':'))
                        break;
                } while (--ofs >= 0);
                ofs++;
                filename += ofs;
                filenameLength -= ofs;
            }
            
            if ((filenameLength == nameLength) && zipStringEqual(filename, zipFileName, filenameLength, false))
            {
                fileIndex = tempFileIndex;
                return true;
            }
        }
        
        return false;
    }
    
    bool ZipFile::GetFileInfo(u32 fileIndex, ZipFile::ZipFileInfo& info, bool* foundExtraData) const
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        const u8* centralDirHeader = &mArchive.state.centralDir.data[mArchive.state.centralDirOffsets.data[fileIndex]];
        
        u32 n;
        const u8* p = centralDirHeader;
        
        if (!p)
        {
            R2_CHECK(false, "Invalid central directory header");
            return false;
        }
        
        info.fileIndex = fileIndex;
        info.cetralDirOffset = mArchive.state.centralDirOffsets.data[fileIndex];
        
        info.versionMadeBy = MZ_READ_LE16(p + R2_ZIP_CDH_VERSION_MADE_BY_OFS);
        info.versionNeeded = MZ_READ_LE16(p + R2_ZIP_CDH_VERSION_NEEDED_OFS);
        info.bitFlag = MZ_READ_LE16(p + R2_ZIP_CDH_BIT_FLAG_OFS);
        info.method = MZ_READ_LE16(p + R2_ZIP_CDH_METHOD_OFS);
        
        info.CRC32 = MZ_READ_LE32(p + R2_ZIP_CDH_CRC32_OFS);
        info.compressedSize = MZ_READ_LE32(p + R2_ZIP_CDH_COMPRESSED_SIZE_OFS);
        info.uncompressedSize = MZ_READ_LE32(p + R2_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
        info.internalAttrib = MZ_READ_LE16(p + R2_ZIP_CDH_INTERNAL_ATTR_OFS);
        info.externalAttrib = MZ_READ_LE32(p + R2_ZIP_CDH_EXTERNAL_ATTR_OFS);
        info.localHeaderOffset = MZ_READ_LE32(p + R2_ZIP_CDH_LOCAL_HEADER_OFS);
        
        n = MZ_READ_LE16(p + R2_ZIP_CDH_FILENAME_LEN_OFS);
        n = MZ_MIN(n, r2::fs::FILE_PATH_LENGTH-1);
        memcpy(info.filename, p + R2_ZIP_CENTRAL_DIR_HEADER_SIZE, n);
        info.filename[n] = '\0';
        
        info.isDirectory = IsFileADirectory(fileIndex);
        info.isEncrypted = false;
        info.isSupported = true; //@TODO(Serge): implement
        
        if (MZ_MAX(MZ_MAX(info.compressedSize, info.uncompressedSize), info.localHeaderOffset) == MZ_UINT32_MAX)
        {
            u32 extraSizeRemaining = MZ_READ_LE16(p + R2_ZIP_CDH_EXTRA_LEN_OFS);
            
            if (extraSizeRemaining)
            {
                const u8* extraData = p + R2_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_READ_LE16(p + R2_ZIP_CDH_FILENAME_LEN_OFS);
                
                do
                {
                    u32 fieldID;
                    u32 fieldDataSize;
                    
                    if (extraSizeRemaining < (sizeof(u16) * 2))
                    {
                        R2_CHECK(false, "Invalid Header or corrupted");
                        return false;
                    }
                    
                    fieldID = MZ_READ_LE16(extraData);
                    fieldDataSize = MZ_READ_LE16(extraData + sizeof(u16));
                    
                    if ((fieldDataSize + sizeof(u16)*2) > extraSizeRemaining)
                    {
                        R2_CHECK(false, "Invalid header or corrupted");
                        return false;
                    }
                    
                    if (fieldID == R2_ZIP64_EXTENDED_INFORMATION_FIELD_HEADER_ID)
                    {
                        const u8* fieldData = extraData + sizeof(u16)*2;
                        u32 fieldDataRemaing = fieldDataSize;
                        
                        if (foundExtraData)
                        {
                            *foundExtraData = true;
                        }
                        
                        if (info.uncompressedSize == MZ_UINT32_MAX)
                        {
                            if (fieldDataRemaing < sizeof(u64))
                            {
                                R2_CHECK(false, "Invalid header or corrupted");
                                return false;
                            }
                            
                            info.uncompressedSize = MZ_READ_LE64(fieldData);
                            fieldData += sizeof(u64);
                            fieldDataRemaing -= sizeof(u64);
                        }
                        
                        if (info.compressedSize == MZ_UINT32_MAX)
                        {
                            if (fieldDataRemaing < sizeof(u64))
                            {
                                R2_CHECK(false, "Invalid header or corrupted");
                                return false;
                            }
                            
                            info.compressedSize = MZ_READ_LE64(fieldData);
                            fieldData += sizeof(u64);
                            fieldDataRemaing -= sizeof(u64);
                        }
                        
                        if (info.localHeaderOffset == MZ_UINT32_MAX)
                        {
                            if (fieldDataRemaing < sizeof(u64))
                            {
                                R2_CHECK(false, "Invalid header or corrupted");
                                return false;
                            }
                            
                            info.localHeaderOffset = MZ_READ_LE64(fieldData);
                            fieldData += sizeof(u64);
                            fieldDataRemaing -= sizeof(u64);
                        }
                        
                        break;
                    }
                    
                    extraData += sizeof(u16)*2 + fieldDataSize;
                    extraSizeRemaining = extraSizeRemaining - sizeof(u16)*2 - fieldDataSize;
                    
                }while(extraSizeRemaining);
            }
        }
        
        return true;
    }
    
    bool ZipFile::ReadUncompressedFileData(void* buffer, u64 bufferSize, u32 fileIndex)
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        if (!buffer || !bufferSize)
        {
            R2_CHECK(false, "Passed in null buffer or 0 sized buffer");
            return false;
        }
        
        s32 status = TINFL_STATUS_DONE;
        
        u64 neededSize, curFileOffset, compRemaining, outBufOffset =0, readBufSize, readBufOffset = 0;
        ZipFile::ZipFileInfo info;
        
        u32 localHeaderU32[(R2_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(u32) - 1)/sizeof(u32)];
        u8* localHeader = (u8*)localHeaderU32;
        
        tinfl_decompressor inflator;
        
        if (!GetFileInfo(fileIndex, info, nullptr))
        {
            return false;
        }
        
        if (info.isDirectory || !info.compressedSize)
        {
            return true;
        }
        
        if (info.bitFlag & (R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_IS_ENCRYPTED | R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_USES_STRONG_ENCRYPTION | R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_COMPRESSED_PATCH_FLAG))
        {
            R2_CHECK(false, "Unsupported encryption");
            return false;
        }
        
        if ((info.method != 0) && (info.method != MZ_DEFLATED))
        {
            R2_CHECK(false, "Unsupported methods");
            return false;
        }
        
        neededSize = info.uncompressedSize;
        if (bufferSize < neededSize)
        {
            R2_CHECK(false, "buffer is too small");
            return false;
        }
        
        curFileOffset = info.localHeaderOffset;
        if (mnoptrFile->Read(localHeader, curFileOffset, R2_ZIP_LOCAL_DIR_HEADER_SIZE) != R2_ZIP_LOCAL_DIR_HEADER_SIZE)
        {
            R2_CHECK(false, "Failed to read file");
            return false;
        }
        
        if (MZ_READ_LE32(localHeader) != R2_ZIP_LOCAL_DIR_HEADER_SIG)
        {
            R2_CHECK(false, "Invalid header or corrupted");
            return false;
        }
        
        curFileOffset += R2_ZIP_LOCAL_DIR_HEADER_SIZE + MZ_READ_LE16(localHeader + R2_ZIP_LDH_FILENAME_LEN_OFS) + MZ_READ_LE16(localHeader + R2_ZIP_LDH_EXTRA_LEN_OFS);
        
        if ((curFileOffset + info.compressedSize) > mArchive.archiveSize)
        {
            R2_CHECK(false, "Invalid header or corrupted");
            return false;
        }
        
        tinfl_init(&inflator);
        
        //temporarily allocate a read buffer
        //@TODO(Serge): replace allocation with some kind of stack allocator - would work well here
        void* readBuf = nullptr;
        constexpr u64 R2_ZIP_MAX_IO_BUF_SIZE = 64 * 1024;
        readBufSize = MZ_MIN(info.compressedSize, (u64)R2_ZIP_MAX_IO_BUF_SIZE);
        if (((sizeof(size_t) == sizeof(u32))) && (readBufSize > 0x7FFFFFFF))
        {
            R2_CHECK(false, "Internal error, read buffer size too big");
            return false;
        }
        
        if (nullptr == (readBuf = mAlloc(readBufSize, alignof(byte))))
        {
            R2_CHECK(false, "Allocation failed");
            return false;
        }
        
        u64 readBufAvail = 0;
        compRemaining = info.compressedSize;
        
        do
        {
            size_t inBufSize, outBufSize = (size_t)(info.uncompressedSize - outBufOffset);
            if ((!readBufAvail))
            {
                readBufAvail = MZ_MIN(readBufSize, compRemaining);
                if (mnoptrFile->Read(readBuf, curFileOffset, readBufAvail) != readBufAvail)
                {
                    status = TINFL_STATUS_FAILED;
                    R2_CHECK(false, "Decompression failed");
                    break;
                }
                curFileOffset += readBufAvail;
                compRemaining -= readBufAvail;
                readBufOffset = 0;
            }
            
            inBufSize = (size_t)readBufAvail;
            status = tinfl_decompress(&inflator, (mz_uint8 *)readBuf + readBufOffset, &inBufSize, (mz_uint8 *)buffer, (mz_uint8 *)buffer + outBufOffset, &outBufSize, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF | (compRemaining ? TINFL_FLAG_HAS_MORE_INPUT : 0));
            readBufAvail -= inBufSize;
            readBufOffset += inBufSize;
            outBufOffset += outBufSize;
            
        }while(status == TINFL_STATUS_NEEDS_MORE_INPUT);
        
        if (status == TINFL_STATUS_DONE)
        {
            if (outBufOffset != info.uncompressedSize)
            {
                R2_CHECK(false, "Unexpected decompressed size");
                 status = TINFL_STATUS_FAILED;
            }
            else if(mz_crc32(MZ_CRC32_INIT, (const mz_uint8*)buffer, (size_t)info.uncompressedSize) != info.CRC32)
            {
                R2_CHECK(false, "CRC check failed");
                status = TINFL_STATUS_FAILED;
            }
        }
        
        mFree((byte*)readBuf);
        
        return status == TINFL_STATUS_DONE;
    }
    
    bool ZipFile::ReadUncompressedFileData(void* buffer, u64 bufferSize, const char* filename)
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        u32 fileIndex;
        if(!FindFile(filename, fileIndex))
        {
            return false;
        }

        return ReadUncompressedFileData(buffer, bufferSize, fileIndex);
    }
    
    bool ZipFile::InitReadArchive()
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        memset(&mArchive, 0, sizeof(ZipArchive));
        mArchive.archiveSize = Size();
        return true;
    }
    
    bool ZipFile::ReadCentralDir()
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        R2_CHECK(mnoptrFile != nullptr, "We shouldn't be calling this method when we don't have a proper file!");
        
        u32 cDirSize = 0;
        u32 cDirEntriesOnThisDisk = 0;
        u32 numThisDisk = 0;
        u32 cDirDiskIndex = 0;
        u64 cDirOffsets = 0;
        u64 curFileOffset = 0;
        constexpr u32 u32BufSize = 4096 / sizeof(u32);
        const u8 * p = nullptr;
        u32 bufU32[u32BufSize];
        u8* pBuf = (u8*)bufU32;
        
        u32 zip64EOCDLSize = (R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIZE + sizeof(u32) - 1) / sizeof(u32);
        
        u32 zip64EndOfCentralDirLocator[zip64EOCDLSize];
        
        u8 * zip64Locator = (u8*)zip64EndOfCentralDirLocator;
        
        u32 zip64EndOfCentralDirHeader[zip64EOCDLSize];
        u8 * zip64EndOfCentralDir = (u8*)zip64EndOfCentralDirHeader;
        
        u64 zip64EndOfCentralDirOffset = 0;
        
        if (mArchive.archiveSize < R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
        {
            R2_CHECK(false, "Not and archive");
            return false;
        }
        
        if (!ZipReaderLocateHeaderSig(R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG, R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE, curFileOffset))
        {
            R2_CHECK(false, "Failed to find central directory!");
            return false;
        }
        
        if (mnoptrFile->Read(pBuf, curFileOffset, R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE) != R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIZE)
        {
            R2_CHECK(false, "Failed to read archive");
            return false;
        }
        
        if (MZ_READ_LE32(pBuf + R2_ZIP_ECDH_SIG_OFS) != R2_ZIP_END_OF_CENTRAL_DIR_HEADER_SIG)
        {
            R2_CHECK(false, "Not an archive");
            return false;
        }
        
        if (curFileOffset >= (R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIZE + R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIZE))
        {
            if (mnoptrFile->Read(zip64Locator, curFileOffset - R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIZE, R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIZE) == R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIZE)
            {
                if (MZ_READ_LE32(zip64Locator + R2_ZIP64_ECDL_SIG_OFS) == R2_ZIP64_END_OF_CENTRAL_DIR_LOCATOR_SIG)
                {
                    zip64EndOfCentralDirOffset = MZ_READ_LE64(zip64Locator + R2_ZIP64_ECDL_REL_OFS_TO_ZIP64_ECDR_OFS);
                    
                    if (zip64EndOfCentralDirOffset > (mArchive.archiveSize - R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIZE))
                    {
                        R2_CHECK(false, "Not an archive");
                        return false;
                    }
                    
                    if (mnoptrFile->Read(zip64EndOfCentralDir, zip64EndOfCentralDirOffset, R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIZE) == R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIZE)
                    {
                        if (MZ_READ_LE32(zip64EndOfCentralDir + R2_ZIP64_ECDH_SIG_OFS) == R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIG)
                        {
                            mArchive.state.isZip64 = true;
                        }
                    }
                }
            }
        }
        
        mArchive.totalFiles = MZ_READ_LE16(pBuf + R2_ZIP_ECDH_CDIR_TOTAL_ENTRIES_OFS);
        cDirEntriesOnThisDisk = MZ_READ_LE16(pBuf + R2_ZIP_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS);
        numThisDisk = MZ_READ_LE16(pBuf + R2_ZIP_ECDH_NUM_THIS_DISK_OFS);
        cDirDiskIndex = MZ_READ_LE16(pBuf + R2_ZIP_ECDH_NUM_DISK_CDIR_OFS);
        cDirSize = MZ_READ_LE32(pBuf + R2_ZIP_ECDH_CDIR_SIZE_OFS);
        cDirOffsets = MZ_READ_LE32(pBuf + R2_ZIP_ECDH_CDIR_OFS_OFS);
        
        if (mArchive.state.isZip64)
        {
            u32 zip64TotalNumOfDisks = MZ_READ_LE32(zip64Locator + R2_ZIP64_ECDL_TOTAL_NUMBER_OF_DISKS_OFS);
            u64 zip64CDirTotalEntries = MZ_READ_LE64(zip64EndOfCentralDir + R2_ZIP64_ECDH_CDIR_TOTAL_ENTRIES_OFS);
            u64 zip64CDirTotalEntriesOnThisDisk = MZ_READ_LE64(zip64EndOfCentralDir + R2_ZIP64_ECDH_CDIR_NUM_ENTRIES_ON_DISK_OFS);
            u64 zip64SizeOfEndOfCentralDirRecord = MZ_READ_LE64(zip64EndOfCentralDir + R2_ZIP64_ECDH_SIZE_OF_RECORD_OFS);
            u64 zip64SizeOfCentralDirectory = MZ_READ_LE64(zip64EndOfCentralDir + R2_ZIP64_ECDH_CDIR_SIZE_OFS);

            if (zip64SizeOfEndOfCentralDirRecord < (R2_ZIP64_END_OF_CENTRAL_DIR_HEADER_SIZE - 12))
            {
                R2_CHECK(false, "Invalid header or corrupted");
                return false;
            }
            
            if (zip64TotalNumOfDisks != 1U)
            {
                R2_CHECK(false, "Unsupported multidisk");
                return false;
            }
            
            if (zip64CDirTotalEntries > MZ_UINT32_MAX)
            {
                R2_CHECK(false, "Too many files!");
                return false;
            }
            
            mArchive.totalFiles = (u32)zip64CDirTotalEntriesOnThisDisk;
            
            if (zip64SizeOfCentralDirectory > MZ_UINT32_MAX)
            {
                R2_CHECK(false, "Unsupported Central Directory size");
                return false;
            }
            
            cDirSize = (u32)zip64SizeOfCentralDirectory;
            
            numThisDisk = MZ_READ_LE32(zip64EndOfCentralDir + R2_ZIP64_ECDH_NUM_THIS_DISK_OFS);
            
            cDirDiskIndex = MZ_READ_LE32(zip64EndOfCentralDir + R2_ZIP64_ECDH_NUM_DISK_CDIR_OFS);
            
            cDirOffsets = MZ_READ_LE64(zip64EndOfCentralDir + R2_ZIP64_ECDH_CDIR_OFS_OFS);
        }
        
        if (mArchive.totalFiles != cDirEntriesOnThisDisk)
        {
            R2_CHECK(false, "Unsupported multidisk");
            return false;
        }
        
        if (((numThisDisk | cDirDiskIndex) != 0) && ((numThisDisk != 1) || (cDirDiskIndex != 1)))
        {
            R2_CHECK(false, "Unsupported multidisk");
            return false;
        }
        
        if (cDirSize < mArchive.totalFiles * R2_ZIP_CENTRAL_DIR_HEADER_SIZE)
        {
            R2_CHECK(false, "Invalid header or corrupted");
            return false;
        }
        
        if ((cDirOffsets + (u64)cDirSize) > mArchive.archiveSize)
        {
            R2_CHECK(false, "Invalid header or corrupted");
            return false;
        }
        
        mArchive.centralDirectoryOffset = cDirOffsets;
        
        if (mArchive.totalFiles)
        {
            u32 i, n;
            if (!AllocateZipArray<byte>(mAlloc, mArchive.state.centralDir, cDirSize, alignof(byte))
                ||!AllocateZipArray<u32>(mAlloc, mArchive.state.centralDirOffsets, mArchive.totalFiles, alignof(u32)))
            {
                R2_CHECK(false, "Failed to allocate our zip arrays!");
            }
            
            //@TODO(Serge): sorting
            
            if (mnoptrFile->Read(mArchive.state.centralDir.data, cDirOffsets, cDirSize) != cDirSize)
            {
                R2_CHECK(false, "Failed to read zip file's central directory");
                return false;
            }
            
            p = (const byte*)mArchive.state.centralDir.data;
            
            for (n = cDirSize, i = 0; i < mArchive.totalFiles; ++i)
            {
                u32 totalHeaderSize, diskIndex, bitFlags, filenameSize, extDataSize;
                u64 compSize, decompSize, localHeaderOffset;
                
                if ((n < R2_ZIP_CENTRAL_DIR_HEADER_SIZE) || (MZ_READ_LE32(p) != R2_ZIP_CENTRAL_DIR_HEADER_SIG))
                {
                    R2_CHECK(false, "Invalid header or corrupted");
                    return false;
                }
                
                mArchive.state.centralDirOffsets.data[i] = (u32)(p - (const byte*)mArchive.state.centralDir.data);
                
                //@TODO(Serge): sorting
                
                compSize = MZ_READ_LE32(p + R2_ZIP_CDH_COMPRESSED_SIZE_OFS);
                decompSize = MZ_READ_LE32(p + R2_ZIP_CDH_DECOMPRESSED_SIZE_OFS);
                localHeaderOffset = MZ_READ_LE32(p + R2_ZIP_CDH_LOCAL_HEADER_OFS);
                filenameSize = MZ_READ_LE16(p + R2_ZIP_CDH_FILENAME_LEN_OFS);
                extDataSize = MZ_READ_LE16(p + R2_ZIP_CDH_EXTRA_LEN_OFS);
                
                if (mArchive.state.zip64HasExtendedInfoFields &&
                    (extDataSize) &&
                    MZ_MAX(MZ_MAX(compSize, decompSize), localHeaderOffset) == MZ_UINT32_MAX)
                {
                    u32 extraSizeRemaining = extDataSize;
                    
                    if (extraSizeRemaining)
                    {
                        const u8* pExtraData;
                        void* buf = nullptr;
                        
                        if (R2_ZIP_CENTRAL_DIR_HEADER_SIZE + filenameSize + extDataSize > n)
                        {
                            //@TODO(Serge): add in file, line number etc
                            buf = mAlloc(extDataSize, alignof(byte));
                            if (buf == nullptr)
                            {
                                R2_CHECK(false, "Failed to allocate buf");
                                return false;
                            }
                            
                            if (mnoptrFile->Read(buf, cDirOffsets + R2_ZIP_CENTRAL_DIR_HEADER_SIZE + filenameSize, extDataSize) != extDataSize)
                            {
                                mFree((byte*)buf);
                                R2_CHECK(false, "Read failed");
                                return false;
                            }
                            
                            pExtraData = (byte*)buf;
                        }
                        else
                        {
                            pExtraData = p + R2_ZIP_CENTRAL_DIR_HEADER_SIZE + filenameSize;
                        }
                        
                        do
                        {
                            u32 fieldId;
                            u32 fieldDataSize;
                            
                            if (extraSizeRemaining < (sizeof(u16)*2))
                            {
                                mFree((byte*)buf);
                                R2_CHECK(false, "Invalid header or corrupted");
                                return false;
                            }
                            
                            fieldId = MZ_READ_LE16(pExtraData);
                            fieldDataSize = MZ_READ_LE16(pExtraData + sizeof(u16));
                            if(fieldDataSize + sizeof(u16)*2 > extraSizeRemaining)
                            {
                                mFree((byte*)buf);
                                R2_CHECK(false, "Invalid header or corrupted");
                                return false;
                            }
                            
                            if (fieldId == R2_ZIP64_EXTENDED_INFORMATION_FIELD_HEADER_ID)
                            {
                                mArchive.state.isZip64 = true;
                                mArchive.state.zip64HasExtendedInfoFields = true;
                                break;
                            }
                            
                            pExtraData += sizeof(u16)*2 + fieldDataSize;
                            extraSizeRemaining = extraSizeRemaining - sizeof(u16)*2 - fieldDataSize;
                            
                        }while (extraSizeRemaining);
                        
                        mFree((byte*)buf);
                    }
                    
                }
                
                /* I've seen archives that aren't marked as zip64 that uses zip64 ext data, argh */
                if ((compSize != MZ_UINT32_MAX) && (decompSize != MZ_UINT32_MAX))
                {
                    if (((!MZ_READ_LE32(p + R2_ZIP_CDH_METHOD_OFS)) && (decompSize != compSize)) || (decompSize && !compSize))
                    {
                        R2_CHECK(false, "Invalid header or corrupted");
                        return false;
                    }
                }
                
                diskIndex = MZ_READ_LE16(p + R2_ZIP_CDH_DISK_START_OFS);
                if ((diskIndex == MZ_UINT16_MAX) || ((diskIndex != numThisDisk) && (diskIndex != 1)))
                {
                    R2_CHECK(false, "Unsupported multi-disk");
                    return false;
                }
                
                if (compSize != MZ_UINT32_MAX)
                {
                    if (((u64)MZ_READ_LE32(p + R2_ZIP_CDH_LOCAL_HEADER_OFS) + R2_ZIP_LOCAL_DIR_HEADER_SIZE + compSize) > mArchive.archiveSize)
                    {
                        R2_CHECK(false, "Invalid header or corrupted");
                        return false;
                    }
                    
                }
                
                bitFlags = MZ_READ_LE16(p + R2_ZIP_CDH_BIT_FLAG_OFS);
                if (bitFlags & R2_ZIP_GENERAL_PURPOSE_BIT_FLAG_LOCAL_DIR_IS_MASKED)
                {
                    R2_CHECK(false, "Unsupported encryption");
                    return false;
                }
                
                if ((totalHeaderSize = R2_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_READ_LE16(p + R2_ZIP_CDH_FILENAME_LEN_OFS) + MZ_READ_LE16(p + R2_ZIP_CDH_EXTRA_LEN_OFS) + MZ_READ_LE16(p + R2_ZIP_CDH_COMMENT_LEN_OFS)) > n)
                {
                    R2_CHECK(false, "Invalid header or corrupted");
                    return false;
                }
                
                
                n -= totalHeaderSize;
                p += totalHeaderSize;
                
            }
            
        }
        
        //@TODO(Serge): sorting
        
        
        return true;
    }
    
    bool ZipFile::ZipReaderLocateHeaderSig(u32 recordSig, u32 recordSize, u64& refCurFileOffset)
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return false;
        }
        
        u64 curFileOffset;
        constexpr u32 bufU32Size = 4096 / sizeof(u32);
        u32 buf[bufU32Size];
        u8* pBuf = (u8*)buf;
        
        if (mArchive.archiveSize < recordSize)
        {
            return false;
        }
        
        curFileOffset = MZ_MAX((u64)mArchive.archiveSize - (s64)sizeof(buf), 0);
        
        for (; ; )
        {
            s32 i, n = (s32)MZ_MIN(sizeof(buf), mArchive.archiveSize - curFileOffset);
            
            if (mnoptrFile->Read(pBuf, curFileOffset, n) != (u64)n)
            {
                return false;
            }
            
            for (i = n - 4; i >= 0; --i)
            {
                u32 s = MZ_READ_LE32(pBuf + i);
                if (s == recordSig)
                {
                    if ((mArchive.archiveSize - (curFileOffset + i)) >= recordSize)
                    {
                        break;
                    }
                }
            }
            
            if (i >= 0)
            {
                curFileOffset += i;
                break;
            }
            
            if ((!curFileOffset) || ((mArchive.archiveSize - curFileOffset) >= (MZ_UINT16_MAX + recordSize)))
            {
                return false;
            }
            
            curFileOffset = MZ_MAX(curFileOffset - (sizeof(buf) - 3), 0);
        }
        
        refCurFileOffset = curFileOffset;
        
        return true;
    }
    
    void ZipFile::EndArchive()
    {
        R2_CHECK(ZIP_FILE_INITIALIZED, "Zip File must be initialized through InitArchive()");
        if (!ZIP_FILE_INITIALIZED)
        {
            return;
        }
        
        auto* fileDevice = mnoptrFile->GetFileDevice();
        fileDevice->Close(mnoptrFile);
        
        FreeZipArray<u8>(mFree, mArchive.state.centralDir);
        FreeZipArray<u32>(mFree, mArchive.state.centralDirOffsets);
    }
}
