//
//  SafeFile.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-22.
//

#include "SafeFile.h"


namespace r2::fs
{
    
    const u8 SafeFile::NUM_CHARS_FOR_HASH;
    
    SafeFile::SafeFile(DiskFileStorageDevice& storageDevice):mnoptrFile(nullptr), mStorageDevice(storageDevice)
    {
        
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
        //make sha256 of file
        //write it out to <filename>.sha
        //write out <filename>.bak
        //close mnoptrFile
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
}
