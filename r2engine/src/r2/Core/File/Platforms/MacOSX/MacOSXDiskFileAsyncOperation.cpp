//
//  MacOSXDiskFileAsyncOperation.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-29.
//

#if defined(R2_PLATFORM_MAC)

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileAsyncOperation.h"

namespace r2::fs
{
    DiskFileAsyncOperation::DiskFileAsyncOperation(FileHandle file, u64 position)
    {
        
    }
    
    DiskFileAsyncOperation::DiskFileAsyncOperation(const DiskFileAsyncOperation& other)
    {
        
    }
    
    DiskFileAsyncOperation& DiskFileAsyncOperation::operator=(const DiskFileAsyncOperation& other)
    {
        return *this;
    }
    
    DiskFileAsyncOperation::~DiskFileAsyncOperation(void)
    {
        
    }
    
    /// Returns whether or not the asynchronous operation has finished
    bool DiskFileAsyncOperation::HasFinished(void) const
    {
        return false;
    }
    
    /// Waits until the asynchronous operation has finished. Returns the number of transferred bytes.
    u64 DiskFileAsyncOperation::WaitUntilFinished(void) const
    {
        return 0;
    }
    
    /// Cancels the asynchronous operation
    void DiskFileAsyncOperation::Cancel(void)
    {
        
    }
}


#endif
