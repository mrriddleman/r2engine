//
//  MacOSXDiskFileAsyncOperation.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-29.
//

#if defined(R2_PLATFORM_MAC)

#include "r2/Core/File/FileDevices/Storage/Disk/DiskFileAsyncOperation.h"
#include "r2/Core/File/FileDevices/Storage/Disk/DiskFile.h"

#include <SDL2/SDL.h>
#include <csignal>
#include <unistd.h>
#include <memory>

namespace
{
    #define IO_SIGNAL SIGUSR1

    const u32 NUM_ACTIVE_REQUESTS = 4096;
    
    static volatile sig_atomic_t gotSIGQUIT = 0;
    static u64 nextRequestId = 0; //maybe should be atomic - I dunno yet
    static u32 openRequests = 0;
    struct sigaction sa;
    
    std::shared_ptr<Request> gRequests[NUM_ACTIVE_REQUESTS];
    
    bool RemoveRequest(const Request& request)
    {
        for (u32 i = 0; i < NUM_ACTIVE_REQUESTS; ++i)
        {
            if (gRequests[i].get() == &request)
            {
                gRequests[i] = nullptr;
                
                openRequests--;
                return true;
            }
        }
        
        R2_CHECK(false, "Remove request failed!");
        
        return false;
    }
    
    bool CancelRequest(Request& request)
    {
        int status = aio_cancel(request.aiocbp.aio_fildes, &request.aiocbp);
        
        if (status == AIO_CANCELED)
        {
            request.status = status;
            return RemoveRequest(request);
        }
        
        return false;
    }
    
    void quitHandler(int sig)
    {
        gotSIGQUIT = 1;
        
        if (openRequests > 0)
        {
            for (u32 i = 0; i < NUM_ACTIVE_REQUESTS; ++i)
            {
                if (gRequests[i] != nullptr)
                {
                    CancelRequest(*gRequests[i]);
                }
            }
        }
    }

    void aioSigHandler(int sig, siginfo_t *si, void *ucontext)
    {
        if (si->si_code == SI_ASYNCIO)
        {
            Request* ioReq = (Request*)si->si_value.sival_ptr; //think about the type - I dunno
            R2_CHECK(ioReq != nullptr, "We don't have an ioReq!");
            
            ioReq->status = AIO_ALLDONE;
            ioReq->returnVal = aio_return(&ioReq->aiocbp);
            
            RemoveRequest(*ioReq);
        }
    }
    
    void InitSig()
    {
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);
        
        sa.sa_handler = quitHandler;
        if (sigaction(SIGQUIT, &sa, NULL) == -1)
            R2_CHECK(false, "Failed to set sigaction for quit");
        
        sa.sa_flags = SA_RESTART | SA_SIGINFO;
        sa.sa_sigaction = aioSigHandler;
        if (sigaction(IO_SIGNAL, &sa, NULL) == -1)
            R2_CHECK(false, "Failed to set sigaction for quit");
    }

    int GetRequestStatus(const Request& request)
    {
        return aio_error(&request.aiocbp);
    }
    
    bool AddRequest(std::shared_ptr<Request> request)
    {
        static std::once_flag initFlag;
        std::call_once(initFlag, InitSig);
        
        //find next empty request slot
        for (u32 i = 0; i < NUM_ACTIVE_REQUESTS; ++i)
        {
            if (!gRequests[i])
            {
                gRequests[i] = request;
                openRequests++;
                return true;
            }
        }
        R2_CHECK(false, "We're full of async requests!");
        return false;
    }

    bool MakeReadRequest(std::shared_ptr<Request>& request, void* buffer, u64 length, u64 position)
    {
        request->reqNum = nextRequestId++;
        request->status = EINPROGRESS;
        request->returnVal = -1;
        
        request->aiocbp.aio_nbytes = length;
        request->aiocbp.aio_reqprio = 0;
        request->aiocbp.aio_offset = position;
        request->aiocbp.aio_buf = buffer;
        request->aiocbp.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
        request->aiocbp.aio_sigevent.sigev_signo = IO_SIGNAL;
        request->aiocbp.aio_sigevent.sigev_value.sival_ptr = request.get();
        
        auto result = aio_read(&request->aiocbp);
        
        if (result != 0)
        {
            return false;
        }
        
        return AddRequest(request);
    }
    
    bool MakeWriteRequest(std::shared_ptr<Request>& request, void* buffer, u64 length, u64 position)
    {
        request->reqNum = nextRequestId++;
        request->status = EINPROGRESS;
        request->returnVal = -1;
        
        request->aiocbp.aio_nbytes = length;
        request->aiocbp.aio_reqprio = 0;
        request->aiocbp.aio_offset = position;
        request->aiocbp.aio_buf = buffer;
        request->aiocbp.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
        request->aiocbp.aio_sigevent.sigev_signo = IO_SIGNAL;
        request->aiocbp.aio_sigevent.sigev_value.sival_ptr = request.get();
        
        auto result = aio_write(&request->aiocbp);
        
        if (result != 0)
        {
            return false;
        }
        
        return AddRequest(request);
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
        if (mRequest->status == AIO_ALLDONE)
        {
            return true;
        }
        
        auto requestStatus = GetRequestStatus(*mRequest);
        
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
        if (mRequest->status == AIO_ALLDONE)
        {
            return mRequest->aiocbp.aio_nbytes;
        }
        
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
