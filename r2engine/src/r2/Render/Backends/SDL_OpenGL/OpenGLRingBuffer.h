#ifndef __OPENGL_BUFFER_H__
#define __OPENGL_BUFFER_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "glad/glad.h"

namespace r2::draw::rendererimpl
{
	struct BufferRange
	{
		u64 startOffset;
		u64 length;
	};

	bool BufferRangeOverlaps(const BufferRange& _lhs, const BufferRange& _rhs);

	struct BufferLock
	{
		BufferRange range;
		GLsync syncObject;
	};

	struct RingBuffer
	{
		r2::SArray<BufferLock>* locks = nullptr;
		void* dataPtr = nullptr;
		GLsizeiptr count = 0;
		GLsizeiptr head = 0;
		GLsizeiptr typeSize = 0;
		GLenum bufferType = 0;
		GLuint bufferName = 0;
		GLuint index = 0;
		CreateConstantBufferFlags flags{0};
	};
	
	//private
	void WaitForLockedRange(r2::SArray<BufferLock>& locks, u64 lockBeginBytes, u64 lockLength);
	void LockRange(r2::SArray<BufferLock>& locks, u64 lockBeginBytes, u64 lockLength);

	namespace ringbuf
	{
		//Ring Buffer Public methods
		template <class ARENA>
		void CreateRingBuffer(ARENA& arena, RingBuffer& ringBuffer, GLuint bufferName, GLenum bufferType, GLsizeiptr typeSize, GLuint index, CreateConstantBufferFlags flags, void* dataPtr, u64 count);
		template <class ARENA>
		void DestroyRingBuffer(ARENA& arena, RingBuffer& ringBuffer);

		void* Reserve(RingBuffer& ringBuffer, u64 count);
		void Complete(RingBuffer& ringBuffer, u64 count);
		GLsizeiptr GetHead(RingBuffer& ringBuffer);
		void* GetHeadOffset(RingBuffer& ringBuffer);
		GLsizeiptr GetSize(const RingBuffer& ringBuffer);
		void BindBufferRange(const RingBuffer& ringBuffer, u32 handle, GLsizeiptr count);
	
		void Clear(RingBuffer& ringBuffer);

		u64 MemorySize(u64 headerSize, u64 boundsChecking, u64 alignment, u64 capacity);
	}
	
	namespace ringbuf
	{
		//Ring Buffer Public methods
		template <class ARENA>
		void CreateRingBuffer(ARENA& arena, RingBuffer& ringBuffer, GLuint bufferName, GLenum bufferType, GLsizeiptr typeSize, GLuint index, CreateConstantBufferFlags flags, void* dataPtr, u64 count)
		{
			ringBuffer.bufferName = bufferName;
			ringBuffer.bufferType = bufferType;
			ringBuffer.dataPtr =  dataPtr;
			ringBuffer.count = count;
			ringBuffer.head = 0;
			ringBuffer.typeSize = typeSize;
			ringBuffer.index = index;
			ringBuffer.locks = MAKE_SARRAY(arena, BufferLock, count);
			ringBuffer.flags = flags;
		}

		template <class ARENA>
		void DestroyRingBuffer(ARENA& arena, RingBuffer& ringBuffer)
		{
			glBindBuffer(ringBuffer.bufferType, ringBuffer.bufferName);
			glUnmapBuffer(ringBuffer.bufferType);

			const u64 numLocks = r2::sarr::Size(*ringBuffer.locks);

			for (u64 i = 0; i < numLocks; ++i)
			{
				auto& lock = r2::sarr::At(*ringBuffer.locks, i);
				glDeleteSync(lock.syncObject);
			}

			r2::sarr::Clear(*ringBuffer.locks);

			FREE(ringBuffer.locks, arena);
			ringBuffer.dataPtr = nullptr;
			ringBuffer.count = 0;
			ringBuffer.head = 0;
		}
	}
}


#endif