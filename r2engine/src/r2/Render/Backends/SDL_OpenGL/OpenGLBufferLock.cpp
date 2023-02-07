#include "r2pch.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLBufferLock.h"

namespace r2::draw::rendererimpl
{
	GLuint64 kOneSecondInNanoSeconds = 100000000;

	bool BufferRangeOverlaps(const BufferRange& _lhs, const BufferRange& _rhs) {
		return _lhs.startOffset < (_rhs.startOffset + _rhs.length)
			&& _rhs.startOffset < (_lhs.startOffset + _lhs.length);
	}

	void Lock(BufferLock& lock)
	{
		lock.syncObject = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		lock.range = { 0, 0 };
	}

	void Wait(BufferLock& lock)
	{
		if (lock.syncObject == nullptr)
		{
			return;
		}

		GLbitfield waitFlags = 0;
		GLuint64 waitDuration = 0;
		while (true) {
			GLenum waitRet = glClientWaitSync(lock.syncObject, waitFlags, waitDuration);

			if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) {
				return;
			}

			if (waitRet == GL_WAIT_FAILED || waitRet == GL_TIMEOUT_EXPIRED) {
				R2_CHECK(false, "Not sure what to do here. Probably crash.");
				return;
			}

			// After the first time, need to start flushing, and wait for a looong time.
			waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
			waitDuration = kOneSecondInNanoSeconds;
		}
	}

	void WaitForLockedRange(r2::SArray<BufferLock>& locks, r2::SArray<BufferLock>& swapLocks, u64 lockBeginBytes, u64 lockLength)
	{
		BufferRange testRange = { lockBeginBytes, lockLength };
		const u64 numLocks = r2::sarr::Size(locks);
		//@Optimization
		//@TODO(Serge): optimize this, too slow
	//	r2::SArray<BufferLock>* swapLocks = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, BufferLock, numLocks);

		for (u64 i = 0; i < numLocks; ++i)
		{
			BufferLock& nextLock = r2::sarr::At(locks, i);
			if (BufferRangeOverlaps(nextLock.range, testRange))
			{
				Wait(nextLock);
				glDeleteSync(nextLock.syncObject);
				nextLock.syncObject = nullptr;
			}
			else
			{
				r2::sarr::Push(swapLocks, nextLock);
			}
		}

		r2::sarr::Clear(locks);
		//@Optimization
		//@TODO(Serge): optimize this, too slow
		r2::sarr::Copy(locks, swapLocks);

		r2::sarr::Clear(swapLocks);

		//FREE(swapLocks, *MEM_ENG_SCRATCH_PTR);
	}

	void LockRange(r2::SArray<BufferLock>& locks, u64 lockBeginBytes, u64 lockLength)
	{
		BufferRange newRange = { lockBeginBytes, lockLength };

		//@Optimization
		//@TODO(Serge): no clue how to make this fast
		GLsync syncName = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		R2_CHECK(glIsSync(syncName), "?");

		BufferLock lock = { {newRange}, syncName };

		r2::sarr::Push(locks, lock);
	}
}