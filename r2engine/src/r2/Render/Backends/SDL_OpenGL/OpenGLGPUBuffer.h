#ifndef __OPENGL_SSBO_LOCK_H__
#define __OPENGL_SSBO_LOCK_H__

#include "r2/Render/Backends/SDL_OpenGL/OpenGLBufferLock.h"
#include "r2/Render/Renderer/BufferLayout.h"

namespace r2::draw::rendererimpl
{
	struct GPUBuffer
	{
		BufferLock bufferLock;
		void* dataPtr = nullptr;
		GLenum bufferType = 0;
		GLuint bufferName = 0;
		GLuint index = 0;
		CreateConstantBufferFlags flags{ 0 };
	};

	namespace gpubuffer
	{
		void BindBuffer(GPUBuffer& gpuBuffer);
		void Complete(GPUBuffer& gpuBuffer);
		void* Reserve(GPUBuffer& gpuBuffer);
	}
}



#endif // __OPENGL_SSBO_LOCK_H__
