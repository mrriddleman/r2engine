//
//  SafeFile.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-22.
//
#include "r2pch.h"
#include "SafeFile.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "sha256.h"
#include <string>

namespace
{
    const char* SHA_EXT = ".sha";
    const char* BACKUP_EXT = ".bak";
}

namespace r2::fs
{
    bool DefaultVerifyFunction(SafeFile& safeFile) {return true;}
    void DefaultVerifyFailedFunction(SafeFile& safeFile){}
    
    const u8 SafeFile::NUM_CHARS_FOR_HASH;
    
    SafeFile::SafeFile(DiskFileStorageDevice& storageDevice):mnoptrFile(nullptr), mStorageDevice(storageDevice), mVerifyFunc(DefaultVerifyFunction), mVerifyFailedFunc(DefaultVerifyFailedFunction)
    {
        strcpy(mSha, "");
    }
    
    SafeFile::SafeFile(DiskFileStorageDevice& storageDevice, SafeFileVerifyFunc fun, SafeFileVerifyFailedFunc failedFunc)
        : mnoptrFile(nullptr)
        , mStorageDevice(storageDevice)
        , mVerifyFunc(fun)
        , mVerifyFailedFunc(failedFunc)
    {
        strcpy(mSha, "");
    }
    
    SafeFile::~SafeFile()
    {
        
    }
    
    bool SafeFile::Open(File* noptrFile)
    {
        R2_CHECK(noptrFile != nullptr, "You passed us in a nullptr!");
        if (noptrFile == nullptr)
        {
            return false;
        }
        
        char fileName[r2::fs::FILE_PATH_LENGTH];
        char filePath[r2::fs::FILE_PATH_LENGTH];
        char fileBackup[r2::fs::FILE_PATH_LENGTH];
        char fileSha[r2::fs::FILE_PATH_LENGTH];
        
        r2::fs::utils::CopyFileName(noptrFile->GetFilePath(), fileName);
        
        r2::fs::utils::CopyDirectoryOfFile(noptrFile->GetFilePath(), filePath);

        r2::fs::utils::AppendSubPath(filePath, fileSha, fileName, r2::fs::utils::PATH_SEPARATOR);
        r2::fs::utils::AppendSubPath(filePath, fileBackup, fileName, r2::fs::utils::PATH_SEPARATOR);
        
        r2::fs::utils::AppendExt(fileSha, SHA_EXT);
        r2::fs::utils::AppendExt(fileBackup, BACKUP_EXT);
        
        if (r2::fs::FileSystem::FileExists(fileSha))
        {
            FileMode mode;
            mode |= Mode::Read;
            
            File* optrShaFile = mStorageDevice.Open(fileSha, mode);
            R2_CHECK(optrShaFile != nullptr, "We should be able to open the file");
            bool result = optrShaFile->ReadAll(mSha);
            R2_CHECK(result, "Should be able to read a file we have already!");
            
            mSha[NUM_CHARS_FOR_HASH-1] = '\0';
            mStorageDevice.Close(optrShaFile);
            
            std::string shaFromShaFile = mSha;
            
            std::string shaFromFile = GetShaFromFile(noptrFile);

            if (shaFromFile == shaFromShaFile)
            {
                mnoptrFile = noptrFile;
                return true;
            }
            else
            {
                if (r2::fs::FileSystem::FileExists(fileBackup))
                {
                    auto mode = noptrFile->GetFileMode();
                    
                    std::string filePathString = noptrFile->GetFilePath();
                    
                    r2::fs::FileSystem::Close(noptrFile);
                    
                    r2::fs::FileSystem::DeleteFile(filePathString.c_str());
                    
                    File* backupFile = mStorageDevice.Open(fileBackup, mode);
                    
                    R2_CHECK(backupFile != nullptr, "We should have a backup");
                    std::string backupSha = GetShaFromFile(backupFile);
                    r2::fs::FileSystem::Close(backupFile);

                    if (backupSha == shaFromShaFile)
                    {
                        r2::fs::FileSystem::RenameFile(fileBackup, filePathString.c_str());
                        r2::fs::FileSystem::CopyFile(filePathString.c_str(), fileBackup);
                        
                        mnoptrFile = mStorageDevice.Open(filePathString.c_str(), mode);
                        return mnoptrFile->IsOpen();
                    }
                    else
                    {
                        R2_CHECK(false, "How was backup not correct?!");
                        //backup is not correct
                        r2::fs::FileSystem::DeleteFile(fileBackup);
                        return false;
                    }
                }
            }
        }
        else
        {
            mnoptrFile = noptrFile;
            return mnoptrFile->IsOpen();
        }
        
        //read the file name
        //check to see if we have a file called <filename>.sha
        //if we do then
            //read the noptrFile file
            //make a sha256 of that file
            //get the sha256 from <filename>.sha
            //compare the two
            //if they are the same then
                //set our mnoptrFile
                //return true
            //else they are not the same
                //something must have been corrupted
                //check to see if we have a backup <filename>.bak
                //if we do then
                    //close noptrFile
                    //read <filename>.bak
                    //make sha256 of <filename>.bak
                    //compare the two
                    //if they are the same
                        //rename <filename>.bak to <filename>
                        //make a copy of <filename>.bak
                        //set mnoptrFile to our new
                    //else
                        //delete <filename>.bak
                        //return false
                //else
                    //return false
        //else we don't have a file called <filename>.sha meaning this is the first time we've ever looked at it
            //return true
        
        return false;
    }
    
    bool SafeFile::Close()
    {
        if (IsOpen())
        {
            char fileName[r2::fs::FILE_PATH_LENGTH];
            char filePath[r2::fs::FILE_PATH_LENGTH];
            char fileBackup[r2::fs::FILE_PATH_LENGTH];
            char fileSha[r2::fs::FILE_PATH_LENGTH];
            
            r2::fs::utils::CopyFileName(mnoptrFile->GetFilePath(), fileName);
            
            r2::fs::utils::CopyDirectoryOfFile(mnoptrFile->GetFilePath(), filePath);
            
            r2::fs::utils::AppendSubPath(filePath, fileSha, fileName, r2::fs::utils::PATH_SEPARATOR);
            r2::fs::utils::AppendSubPath(filePath, fileBackup, fileName, r2::fs::utils::PATH_SEPARATOR);
            
            r2::fs::utils::AppendExt(fileSha, SHA_EXT);
            r2::fs::utils::AppendExt(fileBackup, BACKUP_EXT);
            
            std::string sha = GetShaFromFile(mnoptrFile);
            std::string shaFromFile = mSha;
            
            if (shaFromFile.empty() || shaFromFile != sha)
            {
                if (mVerifyFunc && mVerifyFunc(*this))
                {
                    FileMode mode;
                    mode |= Mode::Write;
                    mode |= Mode::Binary;
                    
                    File* shaFile = mStorageDevice.Open(fileSha, mode);
                    shaFile->Write(sha.c_str(), sha.length());
                    mStorageDevice.Close(shaFile);
                    
                    r2::fs::FileSystem::DeleteFile(fileBackup);
                    r2::fs::FileSystem::CopyFile(mnoptrFile->GetFilePath(), fileBackup);
                }
                else if (mVerifyFailedFunc)
                {
                    mVerifyFailedFunc(*this);
                }
            }
            
            FileStorageDevice* device = mnoptrFile->GetFileDevice();
            device->Close(mnoptrFile);
            mnoptrFile = nullptr;
            return true;
            //make sha256 of file
            //write it out to <filename>.sha
            //write out <filename>.bak
            //close mnoptrFile
        }

        return false;
    }
    
    u64 SafeFile::Read(void* buffer, u64 length)
    {
        return mnoptrFile->Read(buffer, length);
    }
    
    u64 SafeFile::Read(void* buffer, u64 offset, u64 length)
    {
        return mnoptrFile->Read(buffer, offset, length);
    }
    
    u64 SafeFile::Write(const void* buffer, u64 length)
    {
        return mnoptrFile->Write(buffer, length);
    }
    
    bool SafeFile::ReadAll(void* buffer)
    {
        return mnoptrFile->ReadAll(buffer);
    }
    
    void SafeFile::Seek(u64 position)
    {
        mnoptrFile->Seek(position);
    }
    
    void SafeFile::SeekToEnd(void)
    {
        mnoptrFile->SeekToEnd();
    }
    
    void SafeFile::Skip(u64 bytes)
    {
        mnoptrFile->Skip(bytes);
    }
    
    s64 SafeFile::Tell(void) const
    {
        return mnoptrFile->Tell();
    }
    
    bool SafeFile::IsOpen() const
    {
        return mnoptrFile->IsOpen();
    }
    
    s64 SafeFile::Size() const
    {
        return mnoptrFile->Size();
    }
    
    std::string SafeFile::GetShaFromFile(File* noptrFile)
    {
        const auto fileSize = noptrFile->Size();
        
        char* fileBuf = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize+1, alignof(char));
        
        bool result = noptrFile->ReadAll(fileBuf);
        R2_CHECK(result, "Should be able to read all of the file");
        fileBuf[fileSize] = '\0';
        
        std::string shaFromFile = sha256(fileBuf);
        
        FREE(fileBuf, *MEM_ENG_SCRATCH_PTR);
        
        return shaFromFile;
    }
}
