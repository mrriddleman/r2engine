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

#if defined(R2_PLATFORM_MAC)
#include <aio.h>
struct Request
{
    s64 reqNum = -1;
    int status = -1;
    aiocb aiocbp;
    int returnVal = -1;
};

#endif

namespace r2::fs
{
    class R2_API DiskFileAsyncOperation
    {
    public:
        
        DiskFileAsyncOperation(FileHandle file);
        DiskFileAsyncOperation(const DiskFileAsyncOperation& other);
        DiskFileAsyncOperation& operator=(const DiskFileAsyncOperation& other);
        ~DiskFileAsyncOperation(void);
        
        DiskFileAsyncOperation(DiskFileAsyncOperation && other) = delete;
        DiskFileAsyncOperation& operator=(DiskFileAsyncOperation && other) = delete;
        
        /// Returns whether or not the asynchronous operation has finished
        bool HasFinished(void) const;
        
        /// Waits until the asynchronous operation has finished. Returns the number of transferred bytes.
        u64 WaitUntilFinished(u32 usleepAmount) const;
        
        /// Cancels the asynchronous operation
        bool Cancel(void);
        
    private:
        
        friend class DiskFile;
        
        FileHandle mHandle;
        std::shared_ptr<Request> mRequest;
    };
}

#endif /* DiskFileAsyncOperation_h */
