#include "r2pch.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLRingBuffer.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLUtils.h"

namespace r2::draw::rendererimpl
{
	namespace ringbuf
	{
		static bool temp = false;

		void* Reserve(RingBuffer& ringBuffer, u64 count)
		{
			GLsizeiptr lockStart = ringBuffer.head;

			if (lockStart + count > ringBuffer.count) {
				// Need to wrap here.
				ringBuffer.head = (ringBuffer.head + count) % ringBuffer.count;
				lockStart = ringBuffer.head;
			}

			WaitForLockedRange(*ringBuffer.locks, *ringBuffer.swapLocks, lockStart, count);
			char* ringData = (char*)ringBuffer.dataPtr;

			return &ringData[lockStart * ringBuffer.typeSize];
		}

		void Complete(RingBuffer& ringBuffer, u64 size)
		{
			LockRange(*ringBuffer.locks, ringBuffer.head, size);

			ringBuffer.head = (ringBuffer.head + size) % ringBuffer.count;
		}

		void Clear(RingBuffer& ringBuffer)
		{
			r2::SArray<BufferLock>& locks = *ringBuffer.locks;
			const u64 numLocks = r2::sarr::Size(locks);

			r2::SArray<BufferLock>* swapLocks = ringBuffer.swapLocks;//MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BufferLock, numLocks);

			for (u64 i = 0; i < numLocks; ++i)
			{
				BufferLock& nextLock = r2::sarr::At(locks, i);

				GLbitfield waitFlags = 0;
				GLuint64 waitDuration = 0;
	
				GLenum waitRet = glClientWaitSync(*&nextLock.syncObject, waitFlags, waitDuration);

				if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {

					glDeleteSync(nextLock.syncObject);
				}
				else
				{
					r2::sarr::Push(*swapLocks, nextLock);
				}
			}

			r2::sarr::Clear(locks);

			r2::sarr::Copy(locks, *swapLocks);

			r2::sarr::Clear(*ringBuffer.swapLocks);
		//	FREE(swapLocks, *MEM_ENG_SCRATCH_PTR);
		}

		GLsizeiptr GetHead(RingBuffer& ringBuffer)
		{
			return ringBuffer.head;
		}

		void* GetHeadOffset(RingBuffer& ringBuffer)
		{
			return (void*)(ringBuffer.head * ringBuffer.typeSize);
		}

		GLsizeiptr GetSize(const RingBuffer& ringBuffer)
		{
			return ringBuffer.head;
		}

		void BindBufferRange(const RingBuffer& ringBuffer, u32 handle, GLsizeiptr count)
		{
			R2_CHECK(ringBuffer.bufferName == handle, "These should be the same?");
			glBindBufferRange(ringBuffer.bufferType, ringBuffer.index, handle, ringBuffer.head * ringBuffer.typeSize, count * ringBuffer.typeSize);
		}

		u64 MemorySize(u64 headerSize, u64 boundsChecking, u64 alignment, u64 capacity)
		{
			return 
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BufferLock>::MemorySize(RingBuffer::MAX_SWAP_LOCKS), alignment, headerSize, boundsChecking)+
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BufferLock>::MemorySize(RingBuffer::MAX_SWAP_LOCKS), alignment, headerSize, boundsChecking);
		}
	}
}