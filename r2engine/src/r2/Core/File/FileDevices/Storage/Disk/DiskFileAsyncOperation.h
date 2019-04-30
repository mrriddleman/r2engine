//
//  DiskFileAsyncOperation.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-29.
//

#ifndef DiskFileAsyncOperation_h
#define DiskFileAsyncOperation_h

#include "r2/Core/File/File.h"
#include <memory>

namespace r2::fs
{
    class R2_API DiskFileAsyncOperation
    {
    public:
        
        DiskFileAsyncOperation(FileHandle file, u64 position);
        DiskFileAsyncOperation(const DiskFileAsyncOperation& other);
        DiskFileAsyncOperation& operator=(const DiskFileAsyncOperation& other);
        ~DiskFileAsyncOperation(void);
        
        DiskFileAsyncOperation(DiskFileAsyncOperation && other) = delete;
        DiskFileAsyncOperation& operator=(DiskFileAsyncOperation && other) = delete;
        
        /// Returns whether or not the asynchronous operation has finished
        bool HasFinished(void) const;
        
        /// Waits until the asynchronous operation has finished. Returns the number of transferred bytes.
        u64 WaitUntilFinished(void) const;
        
        /// Cancels the asynchronous operation
        void Cancel(void);
        
    private:
        FileHandle mHandle;
    };
}

#endif /* DiskFileAsyncOperation_h */
