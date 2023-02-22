#include "r2pch.h"
#include "r2/Render/Renderer/GPUBuffer.h"
#include "r2/Core/Memory/Memory.h"

namespace r2::draw::vb
{
	using FreeNode = SinglyLinkedList<vb::GPUBufferEntry>::Node;
	static u32 nodeSize = sizeof(SinglyLinkedList<vb::GPUBufferEntry>::Node);

	u64 GPUBuffer::MemorySize(u32 maxNumberOfEntries, u32 alignment, u32 headerSize, u32 boundsChecking)
	{
		u64 size = 0;

		u64 poolHeaderSize = r2::mem::PoolAllocator::HeaderSize();

		size =
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::PoolArena), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(nodeSize, alignment, poolHeaderSize, boundsChecking) * maxNumberOfEntries +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SinglyLinkedList<GPUBufferEntry>::MemorySize(maxNumberOfEntries), alignment, poolHeaderSize, boundsChecking); //overestimate by sizeof(SinglyLinkedList) I think

		return size;
	}
}

namespace r2::draw::vb::gpubuf
{
	void FindBufferEntry(const GPUBuffer& gpuBuffer, u32 size, FreeNode*& previousNode, FreeNode*& foundNode);
	void Coalescence(GPUBuffer& gpuBuffer, FreeNode* previousNode, FreeNode* freeNode);

	void Init(GPUBuffer& gpuBuffer, r2::mem::PoolArena* poolArena, u32 gpuBufferCapacity)
	{
		gpuBuffer.poolArena = poolArena;
		gpuBuffer.bufferCapacity = gpuBufferCapacity;
		gpuBuffer.bufferSize = 0;
		gpuBuffer.bufferPeakUsage = 0;
		gpuBuffer.numberOfGrows = 0;

		FreeNode* node = ALLOC(FreeNode, *poolArena);

		node->data.size = gpuBufferCapacity;
		node->data.start = 0;
		node->next = nullptr;
		gpuBuffer.gpuFreeList.head = nullptr;

		sll::Insert(gpuBuffer.gpuFreeList, nullptr, node);
	}

	void Shutdown(GPUBuffer& gpuBuffer)
	{
		R2_CHECK(gpuBuffer.bufferSize == 0, "We still have allocated memory in the GPUBuffer!");
		FREE(gpuBuffer.gpuFreeList.head, *gpuBuffer.poolArena);
	}

	bool AllocateEntry(GPUBuffer& gpuBuffer, u32 size, GPUBufferEntry& newEntry)
	{
		FreeNode* affectedNode, *previousNode;

		FindBufferEntry(gpuBuffer, size, previousNode, affectedNode);
		
		bool neededToGrow = false;

		while (!affectedNode)
		{
			bool growResult = GrowBuffer(gpuBuffer, gpuBuffer.bufferCapacity * 2);

			if (growResult)
			{
				neededToGrow = true;
			}

			FindBufferEntry(gpuBuffer, size, previousNode, affectedNode);
		}
		
		newEntry.start = affectedNode->data.start;
		newEntry.size = size;

		gpuBuffer.bufferSize += size;
		gpuBuffer.bufferPeakUsage = std::max(gpuBuffer.bufferPeakUsage, gpuBuffer.bufferSize);

		GPUBufferEntry leftOverAmount;
		leftOverAmount.start = affectedNode->data.start + size;
		leftOverAmount.size = affectedNode->data.size - size;

		if (leftOverAmount.size > 0)
		{
			FreeNode* newFreeNode = ALLOC(FreeNode, *gpuBuffer.poolArena);
			newFreeNode->data = leftOverAmount;
			newFreeNode->next = nullptr;

			r2::sll::Insert(gpuBuffer.gpuFreeList, affectedNode, newFreeNode);
		}

		r2::sll::Remove(gpuBuffer.gpuFreeList, previousNode, affectedNode);
		FREE(affectedNode, *gpuBuffer.poolArena);

		return neededToGrow;
	}

	void DeleteEntry(GPUBuffer& gpuBuffer, const GPUBufferEntry& entry)
	{
		FreeNode* it = gpuBuffer.gpuFreeList.head;
		FreeNode* itPrev = nullptr;

		FreeNode* freeNode = ALLOC(FreeNode, *gpuBuffer.poolArena);
		freeNode->next = nullptr;
		freeNode->data = entry;
		bool found = false;
		while (it != nullptr)
		{
			if ((entry.start + entry.size) <= it->data.start)
			{
				found = true;
				r2::sll::Insert(gpuBuffer.gpuFreeList, itPrev, freeNode);
				break;
			}

			itPrev = it;
			it = it->next;
		}

		if (!found && (entry.start + entry.size) <= gpuBuffer.bufferCapacity)
		{
			r2::sll::Insert(gpuBuffer.gpuFreeList, itPrev, freeNode);
		}

		gpuBuffer.bufferSize -= freeNode->data.size;

		Coalescence(gpuBuffer, itPrev, freeNode);
	}

	void FindBufferEntry(const GPUBuffer& gpuBuffer, u32 requiredSize, FreeNode*& previousNode, FreeNode*& foundNode)
	{
		u32 smallestDiff = std::numeric_limits<u32>::max();

		FreeNode* bestBlock = nullptr;
		FreeNode* bestBlockPrev = nullptr;
		FreeNode* it = gpuBuffer.gpuFreeList.head;
		FreeNode* itPrev = nullptr;

		while (it != nullptr)
		{
			if (it->data.size >= requiredSize && ((it->data.size - requiredSize) < smallestDiff))
			{
				bestBlock = it;
				bestBlockPrev = itPrev;
				smallestDiff = it->data.size - requiredSize;
			}

			itPrev = it;
			it = it->next;
		}

		previousNode = bestBlockPrev;
		foundNode = bestBlock;
	}

	bool GrowBuffer(GPUBuffer& gpuBuffer, u32 newCapacity)
	{
		if (newCapacity == 0)
		{
			return false;
		}

		gpuBuffer.numberOfGrows++;

		//find the node that goes to the furthest end of the buffer
		//if it does go to the end of the buffer then tack on more capacity to the size
		//if something is in the last position, then add a new free node

		FreeNode* it = gpuBuffer.gpuFreeList.head;
		FreeNode* itPrev = nullptr;
		FreeNode* lastNode = nullptr;

		while (it != nullptr)
		{
			if (it->data.start + it->data.size == gpuBuffer.bufferCapacity)
			{
				lastNode = it;
				break;
			}

			itPrev = it;
			it = it->next;
		}

		if (lastNode)
		{
			lastNode->data.size += newCapacity - gpuBuffer.bufferCapacity;
		}
		else
		{
			//make a new one
			FreeNode* newCapacityNode = ALLOC(FreeNode, *gpuBuffer.poolArena);
			newCapacityNode->next = nullptr;
			newCapacityNode->data.start = gpuBuffer.bufferCapacity;
			newCapacityNode->data.size = newCapacity - gpuBuffer.bufferCapacity;

			sll::Insert(gpuBuffer.gpuFreeList, itPrev, newCapacityNode);
		}

		gpuBuffer.bufferCapacity = newCapacity;

		return true;
	}

	void FreeAll(GPUBuffer& gpuBuffer)
	{
		gpuBuffer.bufferSize = 0;
		
		RESET_ARENA(*gpuBuffer.poolArena);

		FreeNode* freeNode = ALLOC(FreeNode, *gpuBuffer.poolArena);
		freeNode->next = nullptr;
		freeNode->data.size = gpuBuffer.bufferCapacity;
		freeNode->data.start = 0;

		gpuBuffer.gpuFreeList.head = nullptr;
		r2::sll::Insert(gpuBuffer.gpuFreeList, nullptr, freeNode);
	}

	void SetNewBufferHandle(GPUBuffer& gpuBuffer, u32 handle)
	{
		gpuBuffer.bufferHandle = handle;
	}

	void Coalescence(GPUBuffer& gpuBuffer, FreeNode* previousNode, FreeNode* freeNode)
	{
		if (freeNode->next != nullptr && (freeNode->data.start + freeNode->data.size) == freeNode->next->data.start)
		{
			freeNode->data.size += freeNode->next->data.size;

			FreeNode* next = freeNode->next;

			r2::sll::Remove(gpuBuffer.gpuFreeList, freeNode, next);

			FREE(next, *gpuBuffer.poolArena);
		}

		if (previousNode != nullptr && (previousNode->data.start + previousNode->data.size == freeNode->data.start))
		{
			previousNode->data.size += freeNode->data.size;

			FreeNode* prev = freeNode;

			r2::sll::Remove(gpuBuffer.gpuFreeList, previousNode, prev);

			FREE(prev, *gpuBuffer.poolArena);
		}
	}
}