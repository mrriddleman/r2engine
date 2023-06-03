#ifndef __VERTEX_BUFFER_LAYOUT_SYSTEM_H__
#define __VERTEX_BUFFER_LAYOUT_SYSTEM_H__

#include "r2/Utils/Utils.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Core/Containers/SinglyLinkedList.h"
#include "r2/Core/Memory/Allocators/FreeListAllocator.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/GPUBuffer.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/RenderMaterials/RenderMaterialCache.h"
#include "r2/Render/Renderer/Shader.h"

namespace r2::draw
{

	template<typename T> struct CommandBucket;
}

namespace r2::draw::vb
{
	using VertexBufferLayoutHandle = s32; //this will be the index into the array
	using GPUModelRefHandle = s64;

	const VertexBufferLayoutHandle InvalidVertexBufferLayoutHandle = -1;

	const GPUModelRefHandle InvalidGPUModelRefHandle = -1;

	struct MeshEntry
	{
		GPUBufferEntry gpuVertexEntry;
		GPUBufferEntry gpuIndexEntry;
		u32 materialIndex;

		Bounds meshBounds;
	};

	struct GPUModelRef
	{
		GPUModelRefHandle gpuModelRefHandle;
		u64 modelHash;
		b32 isAnimated;

		r2::SArray<MeshEntry>* meshEntries;
		r2::SArray<rmat::GPURenderMaterialHandle>* renderMaterialHandles;
		r2::SArray<ShaderHandle>* shaderHandles;

		u32 numMaterials;
		GPUBufferEntry boneEntry;
		u32 numBones;
	};

	constexpr size_t MAX_VBOS = BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS;

	struct GPUVertexLayout
	{
		BufferLayoutHandle vaoHandle;
		VertexBufferHandle vboHandles[MAX_VBOS];
		IndexBufferHandle iboHandle;
		DrawIDHandle drawIDHandle;
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

		r2::SArray<GPUModelRef*>* gpuModelRefs;

		r2::mem::FreeListArena* gpuModelRefArena;
	};

	struct VertexBufferLayoutSystem
	{
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;

		u32 mMaxModelsLoaded;
		u32 mAvgNumberOfMeshesPerModel;

		r2::mem::LinearArena* mArena; //if we want more flexibility - make a free list arena instead
		r2::mem::StackArena* mVertexBufferLayoutArena;
		r2::SArray<VertexBufferLayout*>* mVertexBufferLayouts;

		static u32 g_GPUModelSalt;
		static u64 MemorySize(u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel, u64 alignment);
	};

	vb::VertexBufferLayoutSystem* CreateVertexBufferLayoutSystem(const r2::mem::MemoryArea::Handle memoryAreaHandle, u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel);
	void FreeVertexBufferLayoutSystem(vb::VertexBufferLayoutSystem* system);
}

namespace r2::draw::vbsys
{	
	vb::VertexBufferLayoutHandle AddVertexBufferLayout(vb::VertexBufferLayoutSystem& system, const r2::draw::BufferLayoutConfiguration& vertexConfig);
	
	BufferLayoutHandle GetBufferLayoutHandle(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);
	VertexBufferHandle GetVertexBufferHandle(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& vblHandle, u32 vboIndex);

	bool IsVertexBufferLayoutHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	vb::VertexBufferLayoutSize GetVertexBufferCapacity(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);
	vb::VertexBufferLayoutSize GetVertexBufferSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);
	vb::VertexBufferLayoutSize GetVertexBufferRemainingSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	vb::GPUModelRefHandle UploadModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model, r2::draw::CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena);
	vb::GPUModelRefHandle UploadAnimModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::AnimModel& model, r2::draw::CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena);
	
	bool UnloadModelFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& modelRefHandle);
	bool UnloadAllModelsFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	vb::GPUModelRefHandle GetModelRefHandle(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model);
	vb::GPUModelRefHandle GetModelRefHandle(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model);

	u32 GetGPUBufferLayoutHandle(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle);

	bool IsModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model);
	bool IsAnimModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model);

	//Should return true if the model is still loaded - false otherwise
	bool IsModelRefHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle);

	const vb::GPUModelRef* GetGPUModelRef(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle);
	vb::GPUModelRef* GetGPUModelRefPtr(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle);

	//Bulk upload options which I think will probably be used for levels/scenes
	bool UploadAllModels(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::SArray<const Model*>& models, r2::SArray<vb::GPUModelRefHandle>& handles, r2::draw::CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena);
	bool UploadAllAnimModels(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::SArray<const AnimModel*>& models, r2::SArray<vb::GPUModelRefHandle>& handles, r2::draw::CommandBucket<key::Basic>* uploadBucket, r2::mem::StackArena* commandBucketArena);
	bool UnloadAllModelRefHandles(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::SArray<vb::GPUModelRefHandle>* handles);
}

#endif
