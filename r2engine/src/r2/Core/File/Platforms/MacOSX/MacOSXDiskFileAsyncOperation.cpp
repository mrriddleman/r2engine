//
//  MacOSXDiskFileAsyncOperation.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-29.
//

#include "r2pch.h"
#if defined(R2_PLATFORM_MAC)

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileAsyncOperation.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"

#include <SDL2/SDL.h>
#include <unistd.h>
#include <memory>

namespace
{
    static u64 nextRequestId = 0; //maybe should be atomic - I dunno yet

    bool CancelRequest(Request& request)
    {
        int status = aio_cancel(request.aiocbp.aio_fildes, &request.aiocbp);
        
        if (status == AIO_CANCELED)
        {
            request.status = status;
            return true;
        }
        
        return false;
    }

    int GetRequestStatus(const Request& request)
    {
        return aio_error(&request.aiocbp);
    }

    bool MakeReadRequest(std::shared_ptr<Request>& request, void* buffer, u64 length, u64 position)
    {
        request->reqNum = nextRequestId++;
        request->status = EINPROGRESS;
        request->returnVal = -1;
        
      //  memset(&request->aiocbp, 0, sizeof(aiocb));
        request->aiocbp.aio_nbytes = length;
        request->aiocbp.aio_reqprio = 0;
        request->aiocbp.aio_offset = position;
        request->aiocbp.aio_buf = buffer;

        auto result = aio_read(&request->aiocbp);
        
        if (result != 0)
        {
            return false;
        }
        
        return true;
    }
    
    bool MakeWriteRequest(std::shared_ptr<Request>& request, void* buffer, u64 length, u64 position)
    {
        request->reqNum = nextRequestId++;
        request->status = EINPROGRESS;
        request->returnVal = -1;
        
     //   memset(&request->aiocbp, 0, sizeof(aiocb));
        request->aiocbp.aio_nbytes = length;
        request->aiocbp.aio_reqprio = 0;
        request->aiocbp.aio_offset = position;
        request->aiocbp.aio_buf = buffer;

        auto result = aio_write(&request->aiocbp);
        
        if (result != 0)
        {
            return false;
        }
        
        return true;
    }
}

namespace r2::fs
{
    DiskFileAsyncOperation::DiskFileAsyncOperation(FileHandle file):mHandle(file), mRequest(std::make_shared<Request>())
    {
        R2_CHECK(file != nullptr, "File should be open at this point");
        SDL_RWops* ops = (SDL_RWops*)mHandle;
        mRequest->aiocbp.aio_fildes = fileno(ops->hidden.stdio.fp);
    }
    
    DiskFileAsyncOperation::DiskFileAsyncOperation(const DiskFileAsyncOperation& other)
    {
        mHandle = other.mHandle;
        mRequest = other.mRequest;
    }
    
    DiskFileAsyncOperation& DiskFileAsyncOperation::operator=(const DiskFileAsyncOperation& other)
    {
        if (this != &other)
        {
            mHandle = other.mHandle;
            mRequest = other.mRequest;
        }
        
        return *this;
    }
    
    DiskFileAsyncOperation::~DiskFileAsyncOperation(void)
    {
        if (mRequest.use_count() == 1)
        {
            Cancel();
        }
        
        mRequest = nullptr;
        mHandle = nullptr;
    }
    
    /// Returns whether or not the asynchronous operation has finished
    bool DiskFileAsyncOperation::HasFinished(void) const
    {
        auto requestStatus = GetRequestStatus(*(mRequest.get()));

        if (requestStatus == 0)
        {
            return true;
        }
        else
        {
            return requestStatus != EINPROGRESS;
        }
    }
    
    /// Waits until the asynchronous operation has finished. Returns the number of transferred bytes.
    u64 DiskFileAsyncOperation::WaitUntilFinished(u32 usleepAmount) const
    {
        while (!HasFinished())
        {
            if (usleepAmount != 0)
            {
                usleep(usleepAmount);
            }
        }
        
        return mRequest->aiocbp.aio_nbytes;
    }
    
    /// Cancels the asynchronous operation
    bool DiskFileAsyncOperation::Cancel(void)
    {
        bool result = false;
        
        if (mRequest)
        {
            int requestStatus = GetRequestStatus(*mRequest);
            
            if (requestStatus == EINPROGRESS)
            {
                result = CancelRequest(*mRequest);
            }
        }
        
        return result;
    }
    
    DiskFileAsyncOperation DiskFile::ReadAsync(void* buffer, u64 length, u64 position)
    {
        DiskFileAsyncOperation op(mHandle);
        MakeReadRequest(op.mRequest, buffer, length, position);
        return op;
    }
    
    DiskFileAsyncOperation DiskFile::WriteAsync(const void* buffer, u64 length, u64 position)
    {
        DiskFileAsyncOperation op(mHandle);
        MakeWriteRequest(op.mRequest, const_cast<void*>(buffer), length, position);
        return op;
    }
}


#endif

