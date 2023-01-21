#ifndef __VERTEX_BUFFER_SYSTEM_H__
#define __VERTEX_BUFFER_SYSTEM_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SinglyLinkedList.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Model/Model.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Render/Renderer/GPUBuffer.h"

namespace r2::draw
{
	struct ModelSystem;
}

namespace r2::draw::vb
{
	using VertexBufferLayoutHandle = u32; //this will be the index into the array
	using GPUModelRefHandle = u64;

	GPUModelRefHandle InvalidModelRefHandle = 0;

	

	struct MeshEntry
	{
		GPUBufferEntry gpuVertexEntry;
		GPUBufferEntry gpuIndexEntry;
		u32 materialIndex;
	};

	struct GPUModelRef
	{
		VertexBufferLayoutHandle vblHandle;
		GPUModelRefHandle gpuModelRefHandle;
		u64 modelHash;
		b32 isAnimated;
		u32 numBones;

		r2::SArray<MeshEntry>* vertexEntries;
		r2::SArray<MaterialHandle>* materialHandles;
	};

	constexpr size_t MAX_VBOS = BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS;

	struct GPUVertexLayout
	{
		u32 vaoHandle;
		u32 vboHandles[MAX_VBOS];
		u32 iboHandle;
		u32 drawIDHandle;
		u32 numVBOHandles;
	};

	struct VertexBufferLayoutSize
	{
		u32 vertexBufferSizes[MAX_VBOS];
		u32 indexBufferSize;
		u32 numVertexBuffers;
	};

	struct VertexBufferLayout
	{
		GPUVertexLayout gpuLayout;
		BufferLayoutConfiguration layout;

		GPUBuffer vertexBuffers[MAX_VBOS];
		GPUBuffer indexBuffer;

		r2::SArray<GPUModelRef>* gpuModelRefs;
	};

	struct VertexBufferLayoutSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		u32 mMaxModelsLoaded;
		u32 mAvgNumberOfMeshesPerModel;

		r2::mem::LinearArena* mArena; //if we want more flexibility - make a free list arena instead
		r2::mem::StackArena* mGPUModelRefHandleArena;
		r2::SArray<VertexBufferLayout*>* mVertexBufferLayouts;

		static u64 MemorySize(u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel, u64 alignment);
	};

	vb::VertexBufferLayoutSystem* CreateVertexBufferSystem(const r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel);
	void FreeVertexBufferSystem(vb::VertexBufferLayoutSystem* system);
}

namespace r2::draw::vbsys
{
	vb::VertexBufferLayoutHandle AddVertexBufferLayout(vb::VertexBufferLayoutSystem& system, const r2::draw::BufferLayoutConfiguration& vertexConfig);

	vb::VertexBufferLayoutSize GetVertexBufferCapacity(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);
	vb::VertexBufferLayoutSize GetVertexBufferSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);
	vb::VertexBufferLayoutSize GetVertexBufferRemainingSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	//@TODO(Serge): Figure out how this will interact with the renderer - ie. will this generate the command to upload the model data or will this call the renderer to do that etc.
	//				or will this just update some internal data based on the model data
	//				Remember the buffer should resize if we're beyond the size
	vb::GPUModelRefHandle UploadModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model);
	vb::GPUModelRefHandle UploadAnimModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::AnimModel& model);
	
	//Somehow the modelRefHandle will be used to figure out which vb::VertexBufferLayoutHandle is being used in
	bool UnloadModelFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& modelRefHandle);

	bool UnloadAllModelsFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	vb::GPUModelRefHandle GetModelRefHandle(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model);
	vb::GPUModelRefHandle GetModelRefHandle(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model);

	u32 GetGPUBufferLayoutHandle(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	bool IsModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model);
	bool IsAnimModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model);

	//Should return true if the model is still loaded - false otherwise
	bool IsModelRefHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle);

	//Bulk upload options which I think will probably be used for levels/scenes
	bool UploadAllModelsInModelSystem(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::ModelSystem& modelSystem, const r2::SArray<vb::GPUModelRefHandle>* handles);
	bool UnloadAllModelsInModelSystem(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const ModelSystem& modelSystem);
}

#endif
