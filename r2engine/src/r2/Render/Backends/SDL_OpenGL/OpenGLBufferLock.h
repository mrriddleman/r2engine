#ifndef __OPENGL_BUFFER_LOCK_H__
#define __OPENGL_BUFFER_LOCK_H__

#include "glad/glad.h"

namespace r2::draw::rendererimpl
{
	struct BufferRange
	{
		u32 startOffset;
		u32 length;
	};

	bool BufferRangeOverlaps(const BufferRange& _lhs, const BufferRange& _rhs);

	struct BufferLock
	{
		BufferRange range = { 0, 0 };
		GLsync syncObject = nullptr;
	};

	void Lock(BufferLock& lock);
	void Wait(BufferLock& lock);

	void WaitForLockedRange(r2::SArray<BufferLock>& locks, r2::SArray<BufferLock>& swapLocks, u64 lockBeginBytes, u64 lockLength);
	void LockRange(r2::SArray<BufferLock>& locks, u64 lockBeginBytes, u64 lockLength);

}

#endif // __OPENGL_BUFFER_LOCK_H__