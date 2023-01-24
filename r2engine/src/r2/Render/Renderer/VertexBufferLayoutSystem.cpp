#include "r2pch.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Renderer/RendererImpl.h"

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

		const u32 freelistHeaderSize = mem::FreeListAllocator::HeaderSize();

		u64 vertexBufferLayoutArenaSize =
		(
			GetGPUModelRefFreeListArenaSize(maxModelsLoaded, avgNumberOfMeshesPerModel) +
			r2::mem::utils::GetMaxMemoryForAllocation(r2::SArray<GPUModelRef*>::MemorySize(maxModelsLoaded), ALIGNMENT, stackHeaderSize, boundsChecking) +
			r2::mem::utils::GetMaxMemoryForAllocation(GPUBuffer::MemorySize(avgNumberOfMeshesPerModel*maxModelsLoaded, ALIGNMENT, freelistHeaderSize, boundsChecking), ALIGNMENT, stackHeaderSize, boundsChecking) * (MAX_VBOS + 1) +
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

		u32 nodeSize = sizeof(SinglyLinkedList<vb::GPUBufferEntry>::Node);

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

		FREE(vertexBufferLayout, *system.mVertexBufferLayoutArena);

		r2::sarr::Pop(*system.mVertexBufferLayouts);

		return true;
	}
}

namespace r2::draw::vbsys
{
	using FreeNode = SinglyLinkedList<vb::GPUBufferEntry>::Node;

	vb::GPUModelRefHandle UploadModelToVertexBufferInternal(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model, const r2::SArray<BoneData>* boneData, const r2::SArray<BoneInfo>* boneInfo);

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

		for (u32 i = 0; i < system.mMaxModelsLoaded; ++i)
		{
			newVertexBufferLayout->gpuModelRefs->mData[i] = nullptr;
		}

		for (u32 i = 0; i < vertexConfig.numVertexConfigs; ++i)
		{
			r2::mem::FreeListArena* freeList = MAKE_FREELIST_ARENA(*system.mVertexBufferLayoutArena, nodeSize * system.mAvgNumberOfMeshesPerModel * system.mMaxModelsLoaded, mem::FIND_BEST);
			vb::gpubuf::Init(newVertexBufferLayout->vertexBuffers[i], freeList, vertexConfig.vertexBufferConfigs[i].bufferSize);
		}

		if (vertexConfig.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			r2::mem::FreeListArena* freeList = MAKE_FREELIST_ARENA(*system.mVertexBufferLayoutArena, nodeSize * system.mAvgNumberOfMeshesPerModel * system.mMaxModelsLoaded, mem::FIND_BEST);
			vb::gpubuf::Init(newVertexBufferLayout->indexBuffer, freeList, vertexConfig.indexBufferConfig.bufferSize);
		}

		//Now do the actual renderimpl setup of the buffers

		//VAOs
		rendererimpl::GenerateBufferLayouts(1, &newVertexBufferLayout->gpuLayout.vaoHandle);

		//VBOs
		rendererimpl::GenerateVertexBuffers(vertexConfig.numVertexConfigs, &newVertexBufferLayout->gpuLayout.vboHandles[0]);
		newVertexBufferLayout->gpuLayout.numVBOHandles = vertexConfig.numVertexConfigs;

		for (size_t i = 0; i < vertexConfig.numVertexConfigs; i++)
		{
			vb::gpubuf::SetNewBufferHandle(newVertexBufferLayout->vertexBuffers[i], newVertexBufferLayout->gpuLayout.vboHandles[i]);
		}

		//IBOs
		newVertexBufferLayout->gpuLayout.iboHandle = EMPTY_BUFFER;
		if (vertexConfig.indexBufferConfig.bufferSize != EMPTY_BUFFER)
		{
			rendererimpl::GenerateIndexBuffers(1, &newVertexBufferLayout->gpuLayout.iboHandle);
			vb::gpubuf::SetNewBufferHandle(newVertexBufferLayout->indexBuffer, newVertexBufferLayout->gpuLayout.iboHandle);
		}

		//Draw IDs
		newVertexBufferLayout->gpuLayout.drawIDHandle = EMPTY_BUFFER;
		if (vertexConfig.useDrawIDs)
		{
			rendererimpl::GenerateVertexBuffers(1, &newVertexBufferLayout->gpuLayout.drawIDHandle);
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
	vb::GPUModelRefHandle UploadModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model)
	{
		return UploadModelToVertexBufferInternal(system, handle, model, nullptr, nullptr);
	}

	vb::GPUModelRefHandle UploadAnimModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::AnimModel& model)
	{
		return UploadModelToVertexBufferInternal(system, handle, model.model, model.boneData, model.boneInfo);
	}

	vb::GPUModelRefHandle UploadModelToVertexBufferInternal(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model, const r2::SArray<BoneData>* boneData, const r2::SArray<BoneInfo>* boneInfo)
	{
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


		vb::GPUModelRef modelRef;

		return vb::InvalidModelRefHandle;
	}

	//Somehow the modelRefHandle will be used to figure out which vb::VertexBufferHandle is being used for it
	bool UnloadModelFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& modelRefHandle)
	{
		return false;
	}

	bool UnloadAllModelsFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle) 
	{
		return false;
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
		vb::VertexBufferLayoutHandle vblHandle;
		u32 gpuModelRefIndex;
		u32 salt;
		vb::DecodeModelRefHandle(handle, vblHandle, gpuModelRefIndex, salt);

		//@NOTE: this works for now since we don't delete layouts ever - will have to change this if we do.
		const auto& vertexBufferLayout = r2::sarr::At(*system.mVertexBufferLayouts, vblHandle);

		if (!(gpuModelRefIndex >= 0 && gpuModelRefIndex < r2::sarr::Size(*vertexBufferLayout->gpuModelRefs)))
		{
			return false;
		}

		const auto* gpuModelRef = r2::sarr::At(*vertexBufferLayout->gpuModelRefs, gpuModelRefIndex);

		return gpuModelRef->gpuModelRefHandle == handle;
	}

	//Bulk upload options which I think will probably be used for levels/scenes
	bool UploadAllModelsInModelSystem(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::ModelSystem& modelSystem, const r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		return false;
	}

	bool UnloadAllModelsInModelSystem(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const ModelSystem& modelSystem)
	{
		return false;
	}
}