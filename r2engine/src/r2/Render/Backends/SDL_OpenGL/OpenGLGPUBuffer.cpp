#include "r2pch.h"

#include "r2/Render/Backends/SDL_OpenGL/OpenGLGPUBuffer.h"
#include "glad/glad.h"

namespace r2::draw::rendererimpl::gpubuffer
{
	void BindBuffer(GPUBuffer& gpuBuffer)
	{
		glBindBuffer(gpuBuffer.bufferType, gpuBuffer.bufferName);
	}

	void Complete(GPUBuffer& gpuBuffer)
	{
		R2_CHECK(gpuBuffer.bufferLock.syncObject == nullptr, "The OpenGL syncObject should be null here!");
		Lock(gpuBuffer.bufferLock);
	}

	void* Reserve(GPUBuffer& gpuBuffer)
	{
		Wait(gpuBuffer.bufferLock);
		
		glDeleteSync(gpuBuffer.bufferLock.syncObject);
		gpuBuffer.bufferLock.syncObject = nullptr;

		return gpuBuffer.dataPtr;
	}
}