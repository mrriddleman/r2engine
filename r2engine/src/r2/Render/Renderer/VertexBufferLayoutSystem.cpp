#include "r2pch.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core//Memory/Allocators/PoolAllocator.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Renderer/RendererImpl.h"
#include "r2/Render/Renderer/CommandBucket.h"
#include "r2/Render/Model/ModelSystem.h"

namespace r2::draw::vb
{
	static u32 g_GPUModelSalt = 0;

	enum : u32
	{
		GPU_MODEL_REF_KEY_BITS_TOTAL = BytesToBits(sizeof(GPUModelRefHandle)),

		GPU_MODEL_REF_KEY_BITS_LAYOUT_INDEX = 0xC,
		GPU_MODEL_REF_KEY_BITS_INDEX = 0x14,
		GPU_MODEL_REF_KEY_BITS_SALT = 0x20,

		KEY_LAYOUT_INDEX_OFFSET = GPU_MODEL_REF_KEY_BITS_TOTAL - GPU_MODEL_REF_KEY_BITS_LAYOUT_INDEX,
		KEY_INDEX_OFFSET = KEY_LAYOUT_INDEX_OFFSET - GPU_MODEL_REF_KEY_BITS_INDEX,
		KEY_SALT_OFFSET = KEY_INDEX_OFFSET - GPU_MODEL_REF_KEY_BITS_SALT
	};

	bool RemoveVertexBufferLayout(VertexBufferLayoutSystem& system, vb::VertexBufferLayoutHandle);

	GPUModelRefHandle GenerateModelRefHandle(VertexBufferLayoutHandle vertexBufferLayoutIndex, u32 modelRefIndex, u32 salt)
	{
		GPUModelRefHandle handle = 0;
		handle |= ENCODE_KEY_VALUE(vertexBufferLayoutIndex, GPU_MODEL_REF_KEY_BITS_LAYOUT_INDEX, KEY_LAYOUT_INDEX_OFFSET);
		handle |= ENCODE_KEY_VALUE(modelRefIndex, GPU_MODEL_REF_KEY_BITS_INDEX, KEY_INDEX_OFFSET);
		handle |= ENCODE_KEY_VALUE(salt, GPU_MODEL_REF_KEY_BITS_SALT, KEY_SALT_OFFSET);

		return handle;
	}

	void DecodeModelRefHandle(GPUModelRefHandle handle, VertexBufferLayoutHandle& vertexBufferLayoutIndex, u32& modelRefIndex, u32& salt)
	{
		vertexBufferLayoutIndex = DECODE_KEY_VALUE(handle, GPU_MODEL_REF_KEY_BITS_LAYOUT_INDEX, KEY_LAYOUT_INDEX_OFFSET);
		modelRefIndex = DECODE_KEY_VALUE(handle, GPU_MODEL_REF_KEY_BITS_INDEX, KEY_INDEX_OFFSET);
		salt = DECODE_KEY_VALUE(handle, GPU_MODEL_REF_KEY_BITS_SALT, KEY_SALT_OFFSET);
	}

	u64 GetGPUModelRefFreeListArenaSize(u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel)
	{
		static const u32 ALIGNMENT = 16;

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		const u32 freelistHeaderSize = mem::FreeListAllocator::HeaderSize();
		const u32 stackHeaderSize = mem::StackAllocator::HeaderSize();

		u64 GPUModelRefFreeListArenaSize =
			(
				(
					r2::mem::utils::GetMaxMemoryForAllocation(sizeof(GPUModelRef), ALIGNMENT, freelistHeaderSize, boundsChecking) +
					r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MeshEntry>::MemorySize(avgNumberOfMeshesPerModel), ALIGNMENT, freelistHeaderSize, boundsChecking) +
					r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<MaterialHandle>::MemorySize(avgNumberOfMeshesPerModel), ALIGNMENT, freelistHeaderSize, boundsChecking)
				) * maxModelsLoaded
			);

		return GPUModelRefFreeListArenaSize;
	}

	u64 GetVertexBufferLayoutArenaSize(u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel)
	{
		static const u32 ALIGNMENT = 16;

		const u32 stackHeaderSize = mem::StackAllocator::HeaderSize();

		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif

		const u32 poolArenaHeaderSize = mem::PoolAllocator::HeaderSize();

		u64 vertexBufferLayoutArenaSize =
		(
			GetGPUModelRefFreeListArenaSize(maxModelsLoaded, avgNumberOfMeshesPerModel) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<GPUModelRef*>::MemorySize(maxModelsLoaded), ALIGNMENT, stackHeaderSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(GPUBuffer::MemorySize(avgNumberOfMeshesPerModel*maxModelsLoaded, ALIGNMENT, poolArenaHeaderSize, boundsChecking), ALIGNMENT, stackHeaderSize, boundsChecking) * (MAX_VBOS + 1) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(VertexBufferLayout), ALIGNMENT, stackHeaderSize, boundsChecking)
		) * numBufferLayouts;

		R2_CHECK(vertexBufferLayoutArenaSize <= Megabytes(32), "Don't want this to go crazy");

		return vertexBufferLayoutArenaSize;
	}

	vb::VertexBufferLayoutSystem* CreateVertexBufferSystem(const r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel)
	{
		R2_CHECK(memoryAreaHandle != r2::mem::MemoryArea::Invalid, "Memory Area handle is invalid");
		if (memoryAreaHandle == r2::mem::MemoryArea::Invalid)
		{
			return nullptr;
		}

		//Get the memory area
		r2::mem::MemoryArea* noptrMemArea = r2::mem::GlobalMemory::GetMemoryArea(memoryAreaHandle);
		R2_CHECK(noptrMemArea != nullptr, "noptrMemArea is null?");
		if (!noptrMemArea)
		{
			return nullptr;
		}

		static const u32 ALIGNMENT = 16;

		u64 unallocatedSpace = noptrMemArea->UnAllocatedSpace();
		u64 memoryNeeded = VertexBufferLayoutSystem::MemorySize(numBufferLayouts, maxModelsLoaded, avgNumberOfMeshesPerModel, ALIGNMENT);
		if (memoryNeeded > unallocatedSpace)
		{
			R2_CHECK(false, "We don't have enough space to allocate a new sub area for this system");
			return nullptr;
		}

		r2::mem::MemoryArea::SubArea::Handle subAreaHandle = noptrMemArea->AddSubArea(memoryNeeded, "Vertex Buffer System");

		R2_CHECK(subAreaHandle != r2::mem::MemoryArea::SubArea::Invalid, "We have an invalid sub area");

		if (subAreaHandle == r2::mem::MemoryArea::SubArea::Invalid)
		{
			return nullptr;
		}

		r2::mem::MemoryArea::SubArea* noptrSubArea = noptrMemArea->GetSubArea(subAreaHandle);
		R2_CHECK(noptrSubArea != nullptr, "noptrSubArea is null");
		if (!noptrSubArea)
		{
			return nullptr;
		}

		//Emplace the linear arena in the subarea
		r2::mem::LinearArena* vertexBufferLayoutLinearArena = EMPLACE_LINEAR_ARENA(*noptrSubArea);

		if (!vertexBufferLayoutLinearArena)
		{
			R2_CHECK(vertexBufferLayoutLinearArena != nullptr, "linearArena is null");
			return nullptr;
		}

		//allocate the MemorySystem
		VertexBufferLayoutSystem* system = ALLOC(VertexBufferLayoutSystem, *vertexBufferLayoutLinearArena);

		R2_CHECK(system != nullptr, "We couldn't allocate the material system!");

		system->mMemoryAreaHandle = memoryAreaHandle;
		system->mSubAreaHandle = subAreaHandle;
		system->mArena = vertexBufferLayoutLinearArena;
		system->mAvgNumberOfMeshesPerModel = avgNumberOfMeshesPerModel;
		system->mMaxModelsLoaded = maxModelsLoaded;
		system->mVertexBufferLayouts = MAKE_SARRAY(*vertexBufferLayoutLinearArena, VertexBufferLayout*, numBufferLayouts);

		R2_CHECK(system->mVertexBufferLayouts != nullptr, "we couldn't allocate the array for vertex buffer layouts?");

		if (!system->mVertexBufferLayouts)
		{
			return nullptr;
		}

		u64 vertexBufferLayoutArenaSize = GetVertexBufferLayoutArenaSize(numBufferLayouts, maxModelsLoaded, avgNumberOfMeshesPerModel);

		system->mVertexBufferLayoutArena = MAKE_STACK_ARENA(*vertexBufferLayoutLinearArena, vertexBufferLayoutArenaSize);

		R2_CHECK(system->mVertexBufferLayoutArena != nullptr, "We couldn't make the gpu model ref handle area");
		
		if (system->mVertexBufferLayoutArena)
		{
			return nullptr;
		}

		return system;
	}

	void FreeVertexBufferSystem(vb::VertexBufferLayoutSystem* system)
	{
		if (!system)
		{
			R2_CHECK(false, "You passed in a null system!");
			return;
		}

		mem::LinearArena* linearArena = system->mArena;

		R2_CHECK(linearArena != nullptr, "The linear arean is nullptr?");

		const auto& numLayouts = static_cast<s64>( r2::sarr::Size(*system->mVertexBufferLayouts) );

		for (s64 i = numLayouts - 1; i >= 0; --i)
		{
			auto& vertexBufferLayout = r2::sarr::At(*system->mVertexBufferLayouts, i);

			const s32 numGPUModelRefs = static_cast<s32>( r2::sarr::Size(*vertexBufferLayout->gpuModelRefs) );

			//@NOTE(Serge): maybe we should already have things unloaded by the time we get here?
			bool success = vbsys::UnloadAllModelsFromVertexBuffer(*system, i);

			R2_CHECK(success, "Couldn't unload all the models from the vertex buffer");

			success = RemoveVertexBufferLayout(*system, i);

			R2_CHECK(success, "Couldn't remove the vertex buffer layout!");
		}
		
		FREE(system->mVertexBufferLayoutArena, *linearArena);

		FREE(system->mVertexBufferLayouts, *linearArena);

		FREE(system, *linearArena);

		FREE_EMPLACED_ARENA(linearArena);
	}

	u64 VertexBufferLayoutSystem::MemorySize(u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel, u64 alignment)
	{
		u32 boundsChecking = 0;
#ifdef R2_DEBUG
		boundsChecking = r2::mem::BasicBoundsChecking::SIZE_FRONT + r2::mem::BasicBoundsChecking::SIZE_BACK;
#endif
		u32 headerSize = r2::mem::LinearAllocator::HeaderSize();

		u32 stackHeaderSize = r2::mem::StackAllocator::HeaderSize();
		u32 freelistHeaderSize = r2::mem::FreeListAllocator::HeaderSize();

	//	u32 nodeSize = sizeof(SinglyLinkedList<vb::GPUBufferEntry>::Node);

		u64 vertexBufferLayoutArenaSize = GetVertexBufferLayoutArenaSize(numBufferLayouts, maxModelsLoaded, avgNumberOfMeshesPerModel);

		u64 memorySize =
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(VertexBufferLayoutSystem), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::LinearArena), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(sizeof(r2::mem::StackArena), alignment, headerSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<VertexBufferLayout*>::MemorySize(numBufferLayouts), alignment, headerSize, boundsChecking) * numBufferLayouts +
			vertexBufferLayoutArenaSize;

		return memorySize;
	}

	bool RemoveVertexBufferLayout(VertexBufferLayoutSystem& system, vb::VertexBufferLayoutHandle handle)
	{
		R2_CHECK(vbsys::IsVertexBufferLayoutHandleValid(system, handle), "Invalid vertex buffer layout handle!");

		R2_CHECK(!r2::sarr::IsEmpty(*system.mVertexBufferLayouts), "We should have something in here before we remove anything!");

		R2_CHECK(static_cast<s32>(handle) == static_cast<s32>(r2::sarr::Size(*system.mVertexBufferLayouts)) - 1, "These should be the same");

		auto& vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, handle);

		if(vertexBufferLayout->indexBuffer.bufferHandle != 0)
		{ 
			rendererimpl::DeleteVertexBuffers(1, &vertexBufferLayout->gpuLayout.drawIDHandle);

			gpubuf::Shutdown(vertexBufferLayout->indexBuffer);

			FREE(vertexBufferLayout->indexBuffer.freeListArena, *system.mVertexBufferLayoutArena);

			rendererimpl::DeleteIndexBuffers(1, &vertexBufferLayout->gpuLayout.iboHandle);
		}

		for (s32 i = vertexBufferLayout->gpuLayout.numVBOHandles - 1; i >= 0; --i)
		{
			gpubuf::Shutdown(vertexBufferLayout->vertexBuffers[i]);

			FREE(vertexBufferLayout->vertexBuffers[i].freeListArena, *system.mVertexBufferLayoutArena);
		}

		rendererimpl::DeleteVertexBuffers(vertexBufferLayout->gpuLayout.numVBOHandles, &vertexBufferLayout->gpuLayout.vboHandles[0]);

		rendererimpl::DeleteBufferLayouts(1, &vertexBufferLayout->gpuLayout.vaoHandle);

		R2_CHECK(r2::sarr::IsEmpty(*vertexBufferLayout->gpuModelRefs), "Should be empty by the time you free this!");

		FREE(vertexBufferLayout->gpuModelRefs, *system.mVertexBufferLayoutArena);

		FREE(vertexBufferLayout->gpuModelRefArena, *system.mVertexBufferLayoutArena);

		FREE(vertexBufferLayout, *system.mVertexBufferLayoutArena);

		r2::sarr::Pop(*system.mVertexBufferLayouts);

		return true;
	}
}

namespace r2::draw::vbsys
{
	using FreeNode = SinglyLinkedList<vb::GPUBufferEntry>::Node;
	s32 FindNextAvailableGPUModelRefIndex(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayout* layout);
	
	vb::GPUModelRefHandle UploadModelToVertexBufferInternal(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model, const r2::SArray<BoneData>* boneData, const r2::SArray<BoneInfo>* boneInfo, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena);

	u64 FillVertexBufferCommand(cmd::FillVertexBuffer* cmd, const Mesh& mesh, VertexBufferHandle handle, u64 offset);
	u64 FillBonesBufferCommand(cmd::FillVertexBuffer* cmd, const r2::SArray<r2::draw::BoneData>* boneData, VertexBufferHandle handle, u64 offset);
	u64 FillIndexBufferCommand(cmd::FillIndexBuffer* cmd, const Mesh& mesh, IndexBufferHandle handle, u64 offset);

	bool IsVertexBufferLayoutHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		const auto& numLayouts = r2::sarr::Size(*system.mVertexBufferLayouts);

		if (handle >= 0 && handle < numLayouts)
			return true;

		return false;
	}

	vb::VertexBufferLayoutHandle AddVertexBufferLayout(vb::VertexBufferLayoutSystem& system, const r2::draw::BufferLayoutConfiguration& vertexConfig)
	{
		vb::VertexBufferLayout* newVertexBufferLayout = ALLOC(vb::VertexBufferLayout, *system.mVertexBufferLayoutArena);

		newVertexBufferLayout->layout = vertexConfig;

		static u32 nodeSize = sizeof(SinglyLinkedList<vb::GPUBufferEntry>::Node);

		const u64 arenaSize = vb::GetGPUModelRefFreeListArenaSize(system.mMaxModelsLoaded, system.mAvgNumberOfMeshesPerModel);
		
		newVertexBufferLayout->gpuModelRefArena = MAKE_FREELIST_ARENA(*system.mVertexBufferLayoutArena, arenaSize, mem::FIND_BEST);
		newVertexBufferLayout->gpuModelRefs = MAKE_SARRAY(*system.mVertexBufferLayoutArena, vb::GPUModelRef*, system.mMaxModelsLoaded);

		vb::GPUModelRef* nullModelRef = nullptr;
		r2::sarr::Fill(*newVertexBufferLayout->gpuModelRefs, nullModelRef);

		for (u32 i = 0; i < vertexConfig.numVertexConfigs; ++i)
		{
			r2::mem::PoolArena* freeList = MAKE_POOL_ARENA(*system.mVertexBufferLayoutArena, nodeSize, system.mAvgNumberOfMeshesPerModel * system.mMaxModelsLoaded);
			vb::gpubuf::Init(newVertexBufferLayout->vertexBuffers[i], freeList, vertexConfig.vertexBufferConfigs[i].bufferSize);
		}
		//#define MAKE_POOL_ARENA(arena, elementSize, capacity)
		if (vertexConfig.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			r2::mem::PoolArena* freeList = MAKE_POOL_ARENA(*system.mVertexBufferLayoutArena, nodeSize, system.mAvgNumberOfMeshesPerModel * system.mMaxModelsLoaded);
			vb::gpubuf::Init(newVertexBufferLayout->indexBuffer, freeList, vertexConfig.indexBufferConfig.bufferSize);
		}

		//Now do the actual renderimpl setup of the buffers

		//VAOs
		rendererimpl::GenerateBufferLayouts(1, &newVertexBufferLayout->gpuLayout.vaoHandle);

		//VBOs
		rendererimpl::GenerateBuffers(vertexConfig.numVertexConfigs, &newVertexBufferLayout->gpuLayout.vboHandles[0]);
		newVertexBufferLayout->gpuLayout.numVBOHandles = vertexConfig.numVertexConfigs;

		for (size_t i = 0; i < vertexConfig.numVertexConfigs; i++)
		{
			vb::gpubuf::SetNewBufferHandle(newVertexBufferLayout->vertexBuffers[i], newVertexBufferLayout->gpuLayout.vboHandles[i]);
		}

		//IBOs
		newVertexBufferLayout->gpuLayout.iboHandle = EMPTY_BUFFER;
		if (vertexConfig.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			rendererimpl::GenerateBuffers(1, &newVertexBufferLayout->gpuLayout.iboHandle);
			vb::gpubuf::SetNewBufferHandle(newVertexBufferLayout->indexBuffer, newVertexBufferLayout->gpuLayout.iboHandle);
		}

		//Draw IDs
		newVertexBufferLayout->gpuLayout.drawIDHandle = EMPTY_BUFFER;
		if (vertexConfig.useDrawIDs)
		{
			rendererimpl::GenerateBuffers(1, &newVertexBufferLayout->gpuLayout.drawIDHandle);
		}

		rendererimpl::SetupBufferLayoutConfiguration(
			vertexConfig,
			newVertexBufferLayout->gpuLayout.vaoHandle,
			newVertexBufferLayout->gpuLayout.vboHandles, newVertexBufferLayout->gpuLayout.numVBOHandles,
			newVertexBufferLayout->gpuLayout.iboHandle,
			newVertexBufferLayout->gpuLayout.drawIDHandle);

		r2::sarr::Push(*system.mVertexBufferLayouts, newVertexBufferLayout);

		return r2::sarr::Size(*system.mVertexBufferLayouts) - 1;
	}

	vb::VertexBufferLayoutSize GetVertexBufferCapacity(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		vb::VertexBufferLayoutSize size;

		const auto& vblayout = r2::sarr::At(*system.mVertexBufferLayouts, handle);

		size.indexBufferSize = vblayout->indexBuffer.bufferCapacity;
		
		for (u32 i = 0; i < vblayout->gpuLayout.numVBOHandles; ++i)
		{
			size.vertexBufferSizes[i] = vblayout->vertexBuffers[i].bufferCapacity;
		}

		size.numVertexBuffers = vblayout->gpuLayout.numVBOHandles;

		return size;
	}

	vb::VertexBufferLayoutSize GetVertexBufferSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		vb::VertexBufferLayoutSize size;

		const auto& vblayout = r2::sarr::At(*system.mVertexBufferLayouts, handle);

		size.indexBufferSize = vblayout->indexBuffer.bufferSize;

		for (u32 i = 0; i < vblayout->gpuLayout.numVBOHandles; ++i)
		{
			size.vertexBufferSizes[i] = vblayout->vertexBuffers[i].bufferSize;
		}

		size.numVertexBuffers = vblayout->gpuLayout.numVBOHandles;

		return size;
	}

	vb::VertexBufferLayoutSize GetVertexBufferRemainingSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		vb::VertexBufferLayoutSize size;

		const auto& vblayout = r2::sarr::At(*system.mVertexBufferLayouts, handle);

		size.indexBufferSize = vblayout->indexBuffer.bufferCapacity - vblayout->indexBuffer.bufferSize;

		for (u32 i = 0; i < vblayout->gpuLayout.numVBOHandles; ++i)
		{
			size.vertexBufferSizes[i] = vblayout->vertexBuffers[i].bufferCapacity - vblayout->vertexBuffers[i].bufferSize;
		}

		size.numVertexBuffers = vblayout->gpuLayout.numVBOHandles;

		return size;
	}

	//@TODO(Serge): Figure out how this will interact with the renderer - ie. will this generate the command to upload the model data or will this call the renderer to do that etc.
	//				or will this just update some internal data based on the model data
	//				Remember the buffer should resize if we're beyond the size
	vb::GPUModelRefHandle UploadModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		return UploadModelToVertexBufferInternal(system, handle, model, nullptr, nullptr, uploadBucket, commandBucketArena);
	}

	vb::GPUModelRefHandle UploadAnimModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::AnimModel& model, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		return UploadModelToVertexBufferInternal(system, handle, model.model, model.boneData, model.boneInfo, uploadBucket, commandBucketArena);
	}

	

	cmd::CopyBuffer* CopyVertexBuffer(vb::VertexBufferLayoutSystem& system, vb::VertexBufferLayout* vertexBufferLayout, u32 vertexBufferIndex, cmd::CopyBuffer* prevCommand, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		//generate new vertex buffer
		u32 vbo;
		r2::draw::rendererimpl::GenerateBuffers(1, &vbo);
		u32 oldSize = vertexBufferLayout->layout.vertexBufferConfigs[vertexBufferIndex].bufferSize;

		vertexBufferLayout->layout.vertexBufferConfigs[vertexBufferIndex].bufferSize = vertexBufferLayout->vertexBuffers[vertexBufferIndex].bufferCapacity;

		//alloocate the buffer for vbo
		r2::draw::rendererimpl::AllocateVertexBuffer(vbo, vertexBufferLayout->layout.vertexBufferConfigs[vertexBufferIndex].bufferSize, vertexBufferLayout->layout.vertexBufferConfigs[vertexBufferIndex].drawType);

		key::Basic uploadKey;
		uploadKey.keyValue = 0;

		cmd::CopyBuffer* copyBuffer = nullptr;
		
		if (!prevCommand)
		{
			copyBuffer = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::CopyBuffer>(*commandBucketArena, *uploadBucket, uploadKey, 0);
		}
		else
		{
			copyBuffer = cmdbkt::AppendCommand<cmd::CopyBuffer, cmd::CopyBuffer, mem::StackArena>(*commandBucketArena, prevCommand, 0);
		}
		
		copyBuffer->readBuffer = vertexBufferLayout->gpuLayout.vboHandles[vertexBufferIndex];
		copyBuffer->writeBuffer = vbo;
		copyBuffer->readOffset = 0;
		copyBuffer->writeOffset = 0;
		copyBuffer->size = oldSize;

		vb::gpubuf::SetNewBufferHandle(vertexBufferLayout->vertexBuffers[vertexBufferIndex], vbo);
		vertexBufferLayout->gpuLayout.vboHandles[vertexBufferIndex] = vbo;

		return copyBuffer;
	}

	cmd::CopyBuffer* CopyIndexBuffer(vb::VertexBufferLayoutSystem& system, vb::VertexBufferLayout* vertexBufferLayout, cmd::CopyBuffer* prevCommand, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		//generate new index buffer
		u32 ibo;
		r2::draw::rendererimpl::GenerateBuffers(1, &ibo);
		u32 oldSize = vertexBufferLayout->layout.indexBufferConfig.bufferSize;

		vertexBufferLayout->layout.indexBufferConfig.bufferSize = vertexBufferLayout->indexBuffer.bufferCapacity;

		//alloocate the buffer for vbo
		r2::draw::rendererimpl::AllocateIndexBuffer(ibo, vertexBufferLayout->layout.indexBufferConfig.bufferSize, vertexBufferLayout->layout.indexBufferConfig.drawType);

		key::Basic uploadKey;
		uploadKey.keyValue = 0;

		cmd::CopyBuffer* copyBuffer = nullptr;

		if (!prevCommand)
		{
			copyBuffer = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::CopyBuffer>(*commandBucketArena, *uploadBucket, uploadKey, 0);
		}
		else
		{
			copyBuffer = cmdbkt::AppendCommand<cmd::CopyBuffer, cmd::CopyBuffer, mem::StackArena>(*commandBucketArena, prevCommand, 0);
		}

		copyBuffer->readBuffer = vertexBufferLayout->gpuLayout.iboHandle;
		copyBuffer->writeBuffer = ibo;
		copyBuffer->readOffset = 0;
		copyBuffer->writeOffset = 0;
		copyBuffer->size = oldSize;

		vb::gpubuf::SetNewBufferHandle(vertexBufferLayout->indexBuffer, ibo);
		vertexBufferLayout->gpuLayout.iboHandle = ibo;

		return copyBuffer;
	}

	vb::GPUModelRefHandle UploadModelToVertexBufferInternal(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model, const r2::SArray<BoneData>* boneData, const r2::SArray<BoneInfo>* boneInfo, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		//maybe we should check to see if we've already uploaded the model?
		vb::GPUModelRefHandle modelRefHandle = GetModelRefHandle(system, model);
		if (modelRefHandle != InvalidModelRefHandle)
		{
			return modelRefHandle;
		}

		bool isValidHandle = IsVertexBufferLayoutHandleValid(system, handle);

		if (!isValidHandle)
		{
			R2_CHECK(false, "Passed in an invalid VertexBufferLayoutHandle: %i", handle);
			return vb::InvalidModelRefHandle;
		}

		u64 numMeshes = r2::sarr::Size(*model.optrMeshes);
		u64 numMaterals = r2::sarr::Size(*model.optrMaterialHandles);

		R2_CHECK(numMeshes >= 1, "We should have at least one mesh");
		R2_CHECK(numMaterals >= 1, "We should have at least one material");

		vb::VertexBufferLayout* vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, handle);

		R2_CHECK(vertexBufferLayout != nullptr, "vertexBufferLayout is nullptr!");

		vb::GPUModelRef* modelRef = ALLOC(vb::GPUModelRef, *vertexBufferLayout->gpuModelRefArena);

		R2_CHECK(modelRef != nullptr, "modelRef is nullptr!");

		modelRef->vertexEntries = MAKE_SARRAY(*vertexBufferLayout->gpuModelRefArena, vb::MeshEntry, numMeshes);

		R2_CHECK(modelRef->vertexEntries != nullptr, "vertexEntries is nullptr!");

		modelRef->materialHandles = MAKE_SARRAY(*vertexBufferLayout->gpuModelRefArena, MaterialHandle, numMaterals);

		R2_CHECK(modelRef->materialHandles != nullptr, "materialHandles is nullptr!");

		for (u32 i = 0; i < numMaterals; ++i)
		{
			//@NOTE(Serge): this is the exact ordering of the models that came from the assimpassetloader, and needs to be the same, if you change that, this needs to change
			r2::sarr::Push(*modelRef->materialHandles, r2::sarr::At(*model.optrMaterialHandles, i));
		}

		for (u64 i = 0; i < numMeshes; ++i)
		{
			r2::sarr::Push(*modelRef->vertexEntries, vb::MeshEntry());
		}

		//r2::sarr::Size(*vertexBufferLayout->gpuModelRefs)
		s32 gpuRefIndex = FindNextAvailableGPUModelRefIndex(system, vertexBufferLayout);
		R2_CHECK(gpuRefIndex != -1, "We couldn't find a slot to put this gpuref in? We might be out of slots?");

		modelRef->gpuModelRefHandle = vb::GenerateModelRefHandle(handle, gpuRefIndex, vb::g_GPUModelSalt++);
		modelRef->modelHash = model.hash;
		modelRef->isAnimated = boneData && boneInfo;

		if (boneInfo)
		{
			modelRef->boneEntry.size = boneInfo->mSize;
		}

		vb::GPUBufferEntry vertexEntry;
		vb::GPUBufferEntry indexEntry;
		u32 numVertices0 = r2::sarr::Size(*model.optrMeshes->mData[0]->optrVertices);
		u32 numIndices0 = r2::sarr::Size(*model.optrMeshes->mData[0]->optrIndices);
		
		u32 totalNumVertices = 0;
		u32 totalNumIndices = 0;

		for (u32 i = 0; i < r2::sarr::Size(*model.optrMeshes); ++i)
		{
			const auto* mesh = r2::sarr::At(*model.optrMeshes, i);
			totalNumVertices += r2::sarr::Size(*mesh->optrVertices);
			totalNumIndices += r2::sarr::Size(*mesh->optrIndices);
		}
		
		u32 totalMeshSizeInBytes = totalNumVertices * sizeof(r2::draw::Vertex);
		u32 totalIndicesSizeInBytes = totalNumIndices * sizeof(u32);

		bool vertexNeedsToGrow = vb::gpubuf::AllocateEntry(vertexBufferLayout->vertexBuffers[0], totalMeshSizeInBytes, vertexEntry);
		bool indexNeedsToGrow = vb::gpubuf::AllocateEntry(vertexBufferLayout->indexBuffer, totalIndicesSizeInBytes, indexEntry);

		bool boneVertexBufferNeedsToGrow = false;
		vb::GPUBufferEntry boneVertexEntry;

		modelRef->boneEntry.start = 0;
		modelRef->boneEntry.size = 0;

		if (boneData)
		{
			u32 boneSizeInBytes = sizeof(r2::draw::BoneData) * r2::sarr::Size(*boneData);
			boneVertexBufferNeedsToGrow = vb::gpubuf::AllocateEntry(vertexBufferLayout->vertexBuffers[1], boneSizeInBytes, boneVertexEntry);

			modelRef->boneEntry.start = (boneVertexEntry.start) / sizeof(BoneData);

			//@TODO(Serge): implement growing here - for now assert...
			//R2_CHECK(boneVertexBufferNeedsToGrow == false, "Unsupported for the moment");
		}

		//@TODO(Serge): implement growing here - for now assert...
		//R2_CHECK(vertexNeedsToGrow == false && indexNeedsToGrow == false, "Unsupported for the moment");

		cmd::CopyBuffer* prevCommand = nullptr;

		r2::SArray<u32>* bufferHandlesToDelete = nullptr;
		constexpr u32 MAX_NUMBER_OF_BUFFERS_TO_DELETE = 5;

		if (vertexNeedsToGrow)
		{
			if (!bufferHandlesToDelete)
			{
				bufferHandlesToDelete = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, MAX_NUMBER_OF_BUFFERS_TO_DELETE);
			}

			r2::sarr::Push(*bufferHandlesToDelete, vertexBufferLayout->gpuLayout.vboHandles[0]);

			prevCommand = CopyVertexBuffer(system, vertexBufferLayout, 0, nullptr, uploadBucket, commandBucketArena);
		}

		if (boneVertexBufferNeedsToGrow)
		{
			if (!bufferHandlesToDelete)
			{
				bufferHandlesToDelete = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, MAX_NUMBER_OF_BUFFERS_TO_DELETE);
			}

			r2::sarr::Push(*bufferHandlesToDelete, vertexBufferLayout->gpuLayout.vboHandles[1]);

			prevCommand = CopyVertexBuffer(system, vertexBufferLayout, 1, prevCommand, uploadBucket, commandBucketArena);
		}

		if (indexNeedsToGrow)
		{
			if (!bufferHandlesToDelete)
			{
				bufferHandlesToDelete = MAKE_SARRAY(*MEM_ENG_SCRATCH_PTR, u32, MAX_NUMBER_OF_BUFFERS_TO_DELETE);
			}

			r2::sarr::Push(*bufferHandlesToDelete, vertexBufferLayout->gpuLayout.iboHandle);

			prevCommand = CopyIndexBuffer(system, vertexBufferLayout, prevCommand, uploadBucket, commandBucketArena);
		}

		//Do the relayout - maybe needs to be a command?
		if (vertexNeedsToGrow || indexNeedsToGrow || boneVertexBufferNeedsToGrow)
		{
			//@NOTE(Serge): I dunno if we would be better off just deleting the VAO and regenerating it?
			r2::draw::rendererimpl::LayoutBuffersForLayoutConfiguration(
				vertexBufferLayout->layout,
				vertexBufferLayout->gpuLayout.vaoHandle,
				vertexBufferLayout->gpuLayout.vboHandles, vertexBufferLayout->gpuLayout.numVBOHandles,
				vertexBufferLayout->gpuLayout.iboHandle,
				vertexBufferLayout->gpuLayout.drawIDHandle);
		}

		r2::sarr::At(*modelRef->vertexEntries, 0).gpuVertexEntry.start = (vertexEntry.start)/sizeof(r2::draw::Vertex); //this is the base vertex
		r2::sarr::At(*modelRef->vertexEntries, 0).gpuVertexEntry.size = numVertices0;
		r2::sarr::At(*modelRef->vertexEntries, 0).gpuIndexEntry.start = (indexEntry.start) / sizeof(u32);
		r2::sarr::At(*modelRef->vertexEntries, 0).gpuIndexEntry.size = numIndices0;
		r2::sarr::At(*modelRef->vertexEntries, 0).meshBounds = r2::sarr::At(*model.optrMeshes, 0)->objectBounds;
		r2::sarr::At(*modelRef->vertexEntries, 0).materialIndex = r2::sarr::At(*model.optrMeshes, 0)->materialIndex;

		//Upload the first mesh
		cmd::FillVertexBuffer* fillVertexCommand = nullptr;

		if (!prevCommand)
		{
			key::Basic uploadKey;
			uploadKey.keyValue = 0;
			fillVertexCommand = cmdbkt::AddCommand<key::Basic, mem::StackArena, cmd::FillVertexBuffer>(*commandBucketArena, *uploadBucket, uploadKey, 0);
		}
		else
		{
			fillVertexCommand = cmdbkt::AppendCommand<cmd::CopyBuffer, cmd::FillVertexBuffer, mem::StackArena>(*commandBucketArena, prevCommand, 0);
		}

		FillVertexBufferCommand(fillVertexCommand, *r2::sarr::At(*model.optrMeshes, 0), vertexBufferLayout->gpuLayout.vboHandles[0], vertexEntry.start);

		cmd::FillVertexBuffer* nextVertexCmd = fillVertexCommand;

		for (u32 i = 1; i < numMeshes; i++)
		{
			u32 numMeshVertices = r2::sarr::Size(*model.optrMeshes->mData[i]->optrVertices);

			r2::sarr::At(*modelRef->vertexEntries, i).gpuVertexEntry.size = numMeshVertices;
			r2::sarr::At(*modelRef->vertexEntries, i).gpuVertexEntry.start = r2::sarr::At(*modelRef->vertexEntries, i - 1).gpuVertexEntry.size + r2::sarr::At(*modelRef->vertexEntries, i - 1).gpuVertexEntry.start;
			r2::sarr::At(*modelRef->vertexEntries, i).materialIndex = r2::sarr::At(*model.optrMeshes, i)->materialIndex;
			r2::sarr::At(*modelRef->vertexEntries, i).meshBounds = r2::sarr::At(*model.optrMeshes, i)->objectBounds;

			u32 vertexOffset = r2::sarr::At(*modelRef->vertexEntries, i).gpuVertexEntry.start * sizeof(r2::draw::Vertex);

			nextVertexCmd = cmdbkt::AppendCommand<cmd::FillVertexBuffer, cmd::FillVertexBuffer, mem::StackArena>(*commandBucketArena, nextVertexCmd, 0);
			FillVertexBufferCommand(nextVertexCmd, *r2::sarr::At(*model.optrMeshes, i), vertexBufferLayout->gpuLayout.vboHandles[0], vertexOffset);
		}

		//Upload the bones
		if (boneData)
		{
			nextVertexCmd = cmdbkt::AppendCommand<cmd::FillVertexBuffer, cmd::FillVertexBuffer, mem::StackArena>(*commandBucketArena, nextVertexCmd, 0);
			FillBonesBufferCommand(nextVertexCmd, boneData, vertexBufferLayout->gpuLayout.vboHandles[1], boneVertexEntry.start);
		}

		//Upload all of the indices of the model
		cmd::FillIndexBuffer* fillIndexCommand = cmdbkt::AppendCommand<cmd::FillVertexBuffer, cmd::FillIndexBuffer, mem::StackArena>(*commandBucketArena, nextVertexCmd, 0);
		FillIndexBufferCommand(fillIndexCommand, *r2::sarr::At(*model.optrMeshes, 0), vertexBufferLayout->gpuLayout.iboHandle, indexEntry.start);

		cmd::FillIndexBuffer* nextIndexCmd = fillIndexCommand;

		for (u32 i = 1; i < numMeshes; ++i)
		{
			u32 numMeshIndices = r2::sarr::Size(*model.optrMeshes->mData[i]->optrIndices);

			r2::sarr::At(*modelRef->vertexEntries, i).gpuIndexEntry.size = numMeshIndices;
			r2::sarr::At(*modelRef->vertexEntries, i).gpuIndexEntry.start = r2::sarr::At(*modelRef->vertexEntries, i - 1).gpuIndexEntry.size + r2::sarr::At(*modelRef->vertexEntries, i - 1).gpuIndexEntry.start;
		
			u32 indexOffset = r2::sarr::At(*modelRef->vertexEntries, i).gpuIndexEntry.start * sizeof(u32);

			nextIndexCmd = cmdbkt::AppendCommand<cmd::FillIndexBuffer, cmd::FillIndexBuffer, mem::StackArena>(*commandBucketArena, nextIndexCmd, 0);
			FillIndexBufferCommand(fillIndexCommand, *r2::sarr::At(*model.optrMeshes, 0), vertexBufferLayout->gpuLayout.iboHandle, indexOffset);
		}

		//Delete the old vbos if needed
		if (bufferHandlesToDelete)
		{
			cmd::DeleteBuffer* deleteBufferCMD = cmdbkt::AppendCommand<cmd::FillIndexBuffer, cmd::DeleteBuffer, mem::StackArena>(*commandBucketArena, nextIndexCmd, sizeof(u32)* r2::sarr::Size(*bufferHandlesToDelete));

			char* auxMemory = cmdpkt::GetAuxiliaryMemory<cmd::DeleteBuffer>(deleteBufferCMD);

			memcpy(auxMemory, bufferHandlesToDelete->mData, sizeof(u32) * r2::sarr::Size(*bufferHandlesToDelete));

			deleteBufferCMD->bufferHandles = (u32*)auxMemory;
			deleteBufferCMD->numBufferHandles = r2::sarr::Size(*bufferHandlesToDelete);

			FREE(bufferHandlesToDelete, *MEM_ENG_SCRATCH_PTR);
		}

		vertexBufferLayout->gpuModelRefs->mData[gpuRefIndex] = modelRef;

		return modelRef->gpuModelRefHandle;
	}

	//Somehow the modelRefHandle will be used to figure out which vb::VertexBufferHandle is being used for it
	bool UnloadModelFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& modelRefHandle)
	{
		//@TODO(Serge): think about whether or not we actually 0 out the GPU memory... Maybe we can in debug but not release + publish?
		vb::VertexBufferLayoutHandle vertexBufferIndex;
		u32 modelRefIndex;
		u32 salt;
		vb::DecodeModelRefHandle(modelRefHandle, vertexBufferIndex, modelRefIndex, salt);

		if (!IsVertexBufferLayoutHandleValid(system, vertexBufferIndex))
		{
			R2_CHECK(false, "Invalid vertex buffer layout handle: %lu\n", vertexBufferIndex);
			return false;
		}

		vb::VertexBufferLayout* vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, vertexBufferIndex);

		R2_CHECK(vertexBufferLayout != nullptr, "The vertex buffer layout at index: %lu is nullptr!", vertexBufferIndex);

		vb::GPUModelRef* modelRef = r2::sarr::At(*vertexBufferLayout->gpuModelRefs, modelRefIndex);

		R2_CHECK(modelRef != nullptr, "modelRef is nullptr!");

		if (modelRef && modelRef->gpuModelRefHandle != modelRefHandle)
		{
			R2_CHECK(false, "We are trying to unload a model that isn't in GPU memory anymore!");
			return false;
		}

		const auto numVertexEntries =  r2::sarr::Size(*modelRef->vertexEntries);

		R2_CHECK(numVertexEntries > 0, "We should have at least 1");

		vb::GPUBufferEntry vertexEntry;
		vertexEntry.start = sizeof(r2::draw::Vertex) * r2::sarr::At(*modelRef->vertexEntries, 0).gpuVertexEntry.start;
		vertexEntry.size = r2::sarr::At(*modelRef->vertexEntries, 0).gpuVertexEntry.size;

		vb::GPUBufferEntry indexEntry;
		indexEntry.start = sizeof(u32) * r2::sarr::At(*modelRef->vertexEntries, 0).gpuIndexEntry.start;
		indexEntry.size = r2::sarr::At(*modelRef->vertexEntries, 0).gpuIndexEntry.size;

		for (size_t i = 1; i < numVertexEntries; i++)
		{
			vertexEntry.size += r2::sarr::At(*modelRef->vertexEntries, i).gpuVertexEntry.size;
			indexEntry.size += r2::sarr::At(*modelRef->vertexEntries, i).gpuIndexEntry.size;
		}

		vertexEntry.size *= sizeof(Vertex);
		indexEntry.size *= sizeof(u32);

		if (modelRef->boneEntry.size > 0)
		{
			vb::GPUBufferEntry boneEntry;
			boneEntry = modelRef->boneEntry;

			boneEntry.size *= sizeof(BoneData);
			boneEntry.start *= sizeof(BoneData);

			vb::gpubuf::DeleteEntry(vertexBufferLayout->vertexBuffers[1], boneEntry);
		}

		vb::gpubuf::DeleteEntry(vertexBufferLayout->indexBuffer, indexEntry);
		vb::gpubuf::DeleteEntry(vertexBufferLayout->vertexBuffers[0], vertexEntry);

		FREE(modelRef->materialHandles, *vertexBufferLayout->gpuModelRefArena);
		FREE(modelRef->vertexEntries, *vertexBufferLayout->gpuModelRefArena);
		FREE(modelRef, *vertexBufferLayout->gpuModelRefArena);

		return true;
	}

	bool UnloadAllModelsFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle) 
	{
		if (!IsVertexBufferLayoutHandleValid(system, handle))
		{
			R2_CHECK(false, "Invalid vertex buffer layout handle: %lu\n", handle);
			return false;
		}

		vb::VertexBufferLayout* vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, handle);
		R2_CHECK(vertexBufferLayout != nullptr, "The vertex buffer layout at index: %lu is nullptr!", handle);

		vb::gpubuf::FreeAll(vertexBufferLayout->indexBuffer);
		for (u32 i = 0; i < vertexBufferLayout->gpuLayout.numVBOHandles; ++i)
		{
			vb::gpubuf::FreeAll(vertexBufferLayout->vertexBuffers[i]);
		}
		
		r2::sarr::Clear(*vertexBufferLayout->gpuModelRefs);
		RESET_ARENA(*vertexBufferLayout->gpuModelRefArena);

		return true;
	}

	vb::GPUModelRefHandle GetModelRefHandle(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model)
	{
		const auto& numLayouts = r2::sarr::Size(*system.mVertexBufferLayouts);

		for (u64 i = 0; i < numLayouts; ++i)
		{
			const auto& layout = r2::sarr::At(*system.mVertexBufferLayouts, i);

			const auto& numModelRefs = r2::sarr::Capacity(*layout->gpuModelRefs);

			for (u64 j = 0; j < numModelRefs; j++)
			{
				const auto& gpuModelRef = r2::sarr::At(*layout->gpuModelRefs, j);

				if (gpuModelRef && gpuModelRef->modelHash == model.hash)
				{
					return gpuModelRef->gpuModelRefHandle;
				}
			}
		}

		return vb::InvalidModelRefHandle;
	}

	vb::GPUModelRefHandle GetModelRefHandle(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model)
	{
		return GetModelRefHandle(system, model.model);
	}

	u32 GetGPUBufferLayoutHandle(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		R2_CHECK(IsVertexBufferLayoutHandleValid(system, handle), "Invalid vertex buffer layout handle passed in");

		const vb::VertexBufferLayout* layout = r2::sarr::At(*system.mVertexBufferLayouts, handle);

		return layout->gpuLayout.vaoHandle;
	}

	bool IsModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model)
	{
		return GetModelRefHandle(system, model) != vb::InvalidModelRefHandle;
	}

	bool IsAnimModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model)
	{
		return GetModelRefHandle(system, model) != vb::InvalidModelRefHandle;
	}

	//Should return true if the model is still loaded - false otherwise
	bool IsModelRefHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle)
	{
		const vb::GPUModelRef* gpuModelRef = GetGPUModelRef(system, handle);

		return gpuModelRef && gpuModelRef->gpuModelRefHandle == handle;
	}

	const vb::GPUModelRef* GetGPUModelRef(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle)
	{
		vb::VertexBufferLayoutHandle vblHandle;
		u32 gpuModelRefIndex;
		u32 salt;
		vb::DecodeModelRefHandle(handle, vblHandle, gpuModelRefIndex, salt);

		R2_CHECK(IsVertexBufferLayoutHandleValid(system, vblHandle), "Invalid vertex buffer handle: %lu", vblHandle);

		//@NOTE: this works for now since we don't delete layouts ever - will have to change this if we do.
		const auto& vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, vblHandle);

		if (!(gpuModelRefIndex >= 0 && gpuModelRefIndex < r2::sarr::Size(*vertexBufferLayout->gpuModelRefs)))
		{
			return false;
		}

		const auto* gpuModelRef = r2::sarr::At(*vertexBufferLayout->gpuModelRefs, gpuModelRefIndex);

		return gpuModelRef;
	}


	bool UploadAllModels(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& vblHandle, r2::SArray<const Model*>* modelsToUpload, r2::SArray<vb::GPUModelRefHandle>* handles, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		if (IsVertexBufferLayoutHandleValid(system, vblHandle))
		{
			R2_CHECK(false, "Invalid vertex buffer handle: %lu", vblHandle);
			return false;
		}

		if (handles == nullptr || !r2::sarr::IsEmpty(*handles))
		{
			R2_CHECK(false, "GPUModelRefHandles is either nullptr or not empty");
			return false;
		}

		if (modelsToUpload == nullptr || r2::sarr::IsEmpty(*modelsToUpload))
		{
			R2_CHECK(false, "modelsToUpload is either nullptr or empty");
			return false;
		}

		vb::VertexBufferLayout* vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, vblHandle);

		R2_CHECK(vertexBufferLayout != nullptr, "The vertexBufferLayout ptr is still nullptr?");

		//normal models
		const auto numModelsToUpload = r2::sarr::Size(*modelsToUpload);

		for (u32 i = 0; i < numModelsToUpload; ++i)
		{
			const Model* model = r2::sarr::At(*modelsToUpload, i);

			vb::GPUModelRefHandle modelRefHandle = UploadModelToVertexBuffer(system, vblHandle, *model, uploadBucket, commandBucketArena);

			r2::sarr::Push(*handles, modelRefHandle);
		}
	}

	bool UploadAllAnimModels(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& vblHandle, r2::SArray<const AnimModel*>* modelsToUpload, r2::SArray<vb::GPUModelRefHandle>* handles, CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena)
	{
		if (IsVertexBufferLayoutHandleValid(system, vblHandle))
		{
			R2_CHECK(false, "Invalid vertex buffer handle: %lu", vblHandle);
			return false;
		}

		if (handles == nullptr || !r2::sarr::IsEmpty(*handles))
		{
			R2_CHECK(false, "GPUModelRefHandles is either nullptr or not empty");
			return false;
		}

		if (modelsToUpload == nullptr || r2::sarr::IsEmpty(*modelsToUpload))
		{
			R2_CHECK(false, "modelsToUpload is either nullptr or empty");
			return false;
		}

		vb::VertexBufferLayout* vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, vblHandle);

		R2_CHECK(vertexBufferLayout != nullptr, "The vertexBufferLayout ptr is still nullptr?");

		//anim models
		const auto numModelsToUpload = r2::sarr::Size(*modelsToUpload);

		for (u32 i = 0; i < numModelsToUpload; ++i)
		{
			const AnimModel* model = r2::sarr::At(*modelsToUpload, i);

			vb::GPUModelRefHandle modelRefHandle = UploadAnimModelToVertexBuffer(system, vblHandle, *model, uploadBucket, commandBucketArena);

			r2::sarr::Push(*handles, modelRefHandle);
		}

		return true;
	}

	bool UnloadAllModelRefHandles(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& vblHandle, r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		if (IsVertexBufferLayoutHandleValid(system, vblHandle))
		{
			R2_CHECK(false, "Invalid vertex buffer handle: %lu", vblHandle);
			return false;
		}

		vb::VertexBufferLayout* vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, vblHandle);

		R2_CHECK(vertexBufferLayout != nullptr, "The vertexBufferLayout ptr is still nullptr?");

		if (handles == nullptr || r2::sarr::IsEmpty(*handles))
		{
			R2_CHECK(false, "GPUModelRefHandles is either nullptr or is empty");
			return false;
		}
		
		const auto numHandles = r2::sarr::Size(*handles);

		for (u32 i = 0; i < numHandles; ++i)
		{
			const auto modelRefHandle = r2::sarr::At(*handles, i);
			bool success = UnloadModelFromVertexBuffer(system, modelRefHandle);

			if (!success)
			{
				const vb::GPUModelRef* modelRef = GetGPUModelRef(system, modelRefHandle);
				R2_CHECK(modelRef != nullptr, "We have an invalid model ref?");
				R2_CHECK(false, "We couldn't unload modelrefhandle with hash: %llu", modelRef->modelHash);
			}
		}

		return true;
	}

	s32 FindNextAvailableGPUModelRefIndex(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayout* layout)
	{
		R2_CHECK(layout != nullptr, "Passed in a nullptr layout?");

		const auto gpuModelRefCapacity = r2::sarr::Capacity(*layout->gpuModelRefs);
		s32 foundIndex = -1;

		for (u32 i = 0; i < gpuModelRefCapacity; ++i)
		{
			vb::GPUModelRef* modelRef = r2::sarr::At(*layout->gpuModelRefs, i);

			if (modelRef == nullptr)
			{
				foundIndex = i;
				break;
			}
		}

		return foundIndex;
	}

	u64 FillVertexBufferCommand(cmd::FillVertexBuffer* cmd, const Mesh& mesh, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return 0;
		}

		const u64 numVertices = r2::sarr::Size(*mesh.optrVertices);

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::draw::Vertex) * numVertices;
		cmd->data = r2::sarr::Begin(*mesh.optrVertices);

		return cmd->dataSize + offset;
	}

	u64 FillBonesBufferCommand(cmd::FillVertexBuffer* cmd, const r2::SArray<r2::draw::BoneData>* boneData, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return 0;
		}

		const u64 numBoneData = r2::sarr::Size(*boneData);

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::draw::BoneData) * numBoneData;
		cmd->data = r2::sarr::Begin(*boneData);

		return cmd->dataSize + offset;
	}

	u64 FillIndexBufferCommand(cmd::FillIndexBuffer* cmd, const Mesh& mesh, IndexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return  0;
		}

		const u64 numIndices = r2::sarr::Size(*mesh.optrIndices);

		cmd->indexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::sarr::At(*mesh.optrIndices, 0)) * numIndices;
		cmd->data = r2::sarr::Begin(*mesh.optrIndices);

		return cmd->dataSize + offset;
	}
}