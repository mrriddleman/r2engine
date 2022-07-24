#ifndef __OPENGL_BUFFER_H__
#define __OPENGL_BUFFER_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "glad/glad.h"
#include "r2/Render/Backends/SDL_OpenGL/OpenGLBufferLock.h"

namespace r2::draw::rendererimpl
{
	struct RingBuffer
	{
		r2::SArray<BufferLock>* locks = nullptr;
		r2::SArray<BufferLock>* swapLocks = nullptr;
		void* dataPtr = nullptr;
		GLsizeiptr count = 0;
		GLsizeiptr head = 0;
		GLsizeiptr typeSize = 0;
		GLenum bufferType = 0;
		GLuint bufferName = 0;
		GLuint index = 0;
		CreateConstantBufferFlags flags{0};

		static const u32 MAX_SWAP_LOCKS = 100;

	};

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
			ringBuffer.locks = MAKE_SARRAY(arena, BufferLock, RingBuffer::MAX_SWAP_LOCKS);
			ringBuffer.swapLocks = MAKE_SARRAY(arena, BufferLock, RingBuffer::MAX_SWAP_LOCKS);
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
			r2::sarr::Clear(*ringBuffer.swapLocks);
			FREE(ringBuffer.swapLocks, arena);
			FREE(ringBuffer.locks, arena);

			ringBuffer.dataPtr = nullptr;
			ringBuffer.count = 0;
			ringBuffer.head = 0;
		}
	}
}


#endif