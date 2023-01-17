#ifndef __VERTEX_BUFFER_SYSTEM_H__
#define __VERTEX_BUFFER_SYSTEM_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SinglyLinkedList.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Model/Model.h"

namespace r2::draw
{
	struct ModelSystem;
}

namespace r2::mem::utils
{
	struct MemBoundary;
}

namespace r2::draw::vb
{
	using VertexBufferLayoutHandle = u32; //this will be the index into the array
	using GPUModelRefHandle = u64;

	struct GPUBufferEntry
	{
		u32 bufferHandle;
		u32 startingOffset;
		u32 size;
	};

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

		u32 numVBOHandles;
	};

	struct GPUBuffer
	{
		u32 bufferHandle;
		u32 bufferSize;
		u32 bufferCapacity;
		r2::SinglyLinkedList<GPUBufferEntry>* gpuFreeList;
	};

	struct VertexBufferLayoutSize
	{
		u32 vertexBufferSize[MAX_VBOS];
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

		r2::mem::LinearArena* mArena;
		r2::mem::StackArena* mGPUModelRefHandleArena;
		r2::SArray<VertexBufferLayout>* mVertexBufferLayouts;

		static u64 MemorySize(u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel, u64 alignment);
	};

	vb::VertexBufferLayoutSystem* CreateVertexBufferSystem(const r2::mem::MemoryArea::Handle memoryAreaHandle);
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
	
	bool IsModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model);
	bool IsAnimModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model);

	//Should return true if the model is still loaded - false otherwise
	bool IsModelRefHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle);

	//Bulk upload options which I think will probably be used for levels/scenes
	bool UploadAllModelsInModelSystem(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::ModelSystem& modelSystem, const r2::SArray<vb::GPUModelRefHandle>* handles);
	bool UnloadAllModelsInModelSystem(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const ModelSystem& modelSystem);
}

#endif
