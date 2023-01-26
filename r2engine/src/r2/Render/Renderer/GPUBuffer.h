#ifndef __GPUBUFFER_H__
#define __GPUBUFFER_H__

#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Core/Containers/SinglyLinkedList.h"

namespace r2::draw::vb
{
	struct GPUBufferEntry
	{
		u32 start; //base vertex/index
		u32 size; //number of vertices/indices
	};

	/*
	The GPUBuffer won't do any renderer implementation stuff. This is only to keep track of what's been allocated in the buffer
	We'll need to do any core rendering impl stuff outside of this struct 
	*/

	struct GPUBuffer
	{
		u32 bufferHandle;
		u32 bufferSize;
		u32 bufferCapacity;
		u32 bufferPeakUsage;
		u32 numberOfGrows;

		//@TODO(Serge): make this a pool instead!
		r2::mem::FreeListArena* freeListArena;

		r2::SinglyLinkedList<GPUBufferEntry> gpuFreeList;


		static u64 MemorySize(u32 maxNumberOfEntries, u32 alignment, u32 headerSize, u32 boundsChecking);
	};
}

namespace r2::draw::vb::gpubuf
{
	void Init(GPUBuffer& gpuBuffer, r2::mem::FreeListArena* freeList, u32 gpuBufferCapacity);
	void Shutdown(GPUBuffer& gpuBuffer);
	bool AllocateEntry(GPUBuffer& gpuBuffer, u32 size, GPUBufferEntry& newEntry);
	void DeleteEntry(GPUBuffer& gpuBuffer, const GPUBufferEntry& entry);
	bool GrowBuffer(GPUBuffer& gpuBuffer, u32 newCapacity);
	void FreeAll(GPUBuffer& gpuBuffer);
	void SetNewBufferHandle(GPUBuffer& gpuBuffer, u32 handle);
}


#endif // __GPUBUFFER_H__
