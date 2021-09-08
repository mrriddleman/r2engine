#include "r2pch.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLRingBuffer.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/Allocators/StackAllocator.h"
#include "r2/Core/Memory/InternalEngineMemory.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLUtils.h"

namespace r2::draw::rendererimpl
{
	GLuint64 kOneSecondInNanoSeconds = 100000000;

	bool BufferRangeOverlaps(const BufferRange& _lhs, const BufferRange& _rhs) {
		return _lhs.startOffset < (_rhs.startOffset + _rhs.length)
			&& _rhs.startOffset < (_lhs.startOffset + _lhs.length);
	}

	void Wait(GLsync* syncObj)
	{
		GLbitfield waitFlags = 0;
		GLuint64 waitDuration = 0;
		while (true) {
			GLenum waitRet = glClientWaitSync(*syncObj, waitFlags, waitDuration);

			if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
				return;
			}

			if (waitRet == GL_WAIT_FAILED) {
				R2_CHECK(false, "Not sure what to do here. Probably raise an exception or something.");
				return;
			}

			// After the first time, need to start flushing, and wait for a looong time.
			waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
			waitDuration = kOneSecondInNanoSeconds;
		}
	}

	void WaitForLockedRange(r2::SArray<BufferLock>& locks, u64 lockBeginBytes, u64 lockLength)
	{
		BufferRange testRange = { lockBeginBytes, lockLength };
		const u64 numLocks = r2::sarr::Size(locks);
		//@Optimization
		//@TODO(Serge): optimize this, too slow
		r2::SArray<BufferLock>* swapLocks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BufferLock, numLocks);

		for (u64 i = 0; i < numLocks; ++i)
		{
			BufferLock& nextLock = r2::sarr::At(locks, i);
			if (BufferRangeOverlaps(nextLock.range, testRange))
			{
				Wait(&nextLock.syncObject);
				glDeleteSync(nextLock.syncObject);
			}
			else 
			{
				r2::sarr::Push(*swapLocks, nextLock);
			}
		}

		r2::sarr::Clear(locks);
		//@Optimization
		//@TODO(Serge): optimize this, too slow
		r2::sarr::Copy(locks, *swapLocks);

		FREE(swapLocks, *MEM_ENG_SCRATCH_PTR);
	}

	void LockRange(r2::SArray<BufferLock>& locks, u64 lockBeginBytes, u64 lockLength)
	{
		BufferRange newRange = { lockBeginBytes, lockLength };
		//@Optimization
		//@TODO(Serge): no clue how to make this fast
		GLsync syncName = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		BufferLock lock = { {newRange}, syncName };

		r2::sarr::Push(locks, lock);
	}

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

			WaitForLockedRange(*ringBuffer.locks, lockStart, count);
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

			r2::SArray<BufferLock>* swapLocks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BufferLock, numLocks);

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

			FREE(swapLocks, *MEM_ENG_SCRATCH_PTR);
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
			glBindBufferRange(ringBuffer.bufferType, ringBuffer.index, handle, ringBuffer.head * ringBuffer.typeSize, count * ringBuffer.typeSize);
		}

		u64 MemorySize(u64 headerSize, u64 boundsChecking, u64 alignment, u64 capacity)
		{
			return r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<BufferLock>::MemorySize(capacity), alignment, headerSize, boundsChecking);
		}
	}
}