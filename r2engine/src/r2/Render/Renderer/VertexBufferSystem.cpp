#include "r2pch.h"
#include "r2/Render/Renderer/VertexBufferSystem.h"

namespace r2::draw::vb
{
	vb::VertexBufferSystem* CreateVertexBufferSystem(const r2::mem::utils::MemBoundary& boundary)
	{
		return nullptr;
	}

	void FreeVertexBufferSystem(vb::VertexBufferSystem* system)
	{

	}
}

namespace r2::draw::vbsys
{
	vb::VertexBufferHandle AddVertexBufferLayout(vb::VertexBufferSystem& system, const r2::draw::BufferLayoutConfiguration& vertexConfig)
	{
		return {};
	}

	u32 GetVertexBufferCapacity(const vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle)
	{
		return 0;
	}

	u32 GetVertexBufferSize(const vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle)
	{
		return 0;
	}

	u32 GetVertexBufferRemainingSize(const vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle)
	{
		return 0;
	}

	//@TODO(Serge): Figure out how this will interact with the renderer - ie. will this generate the command to upload the model data or will this call the renderer to do that etc.
	//				or will this just update some internal data based on the model data
	//				Remember the buffer should resize if we're beyond the size
	vb::GPUModelRefHandle UploadModelToVertexBuffer(vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle, const r2::draw::Model& model)
	{
		return {};
	}

	vb::GPUModelRefHandle UploadAnimModelToVertexBuffer(vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle, const r2::draw::AnimModel& model)
	{
		return {};
	}

	//Somehow the modelRefHandle will be used to figure out which vb::VertexBufferHandle is being used for it
	bool UnloadModelFromVertexBuffer(vb::VertexBufferSystem& system, const vb::GPUModelRefHandle& modelRefHandle)
	{
		return false;
	}

	bool IsModelLoaded(const vb::VertexBufferSystem& system, const r2::draw::Model& model)
	{
		return false;
	}

	bool IsAnimModelLoaded(const vb::VertexBufferSystem& system, const r2::draw::AnimModel& model)
	{
		return false;
	}


	//Should return true if the model is still loaded - false otherwise
	bool IsModelRefHandleValid(const vb::VertexBufferSystem& system, const vb::GPUModelRefHandle& handle)
	{
		return false;
	}


	//Bulk upload options which I think will probably be used for levels/scenes
	bool UploadAllModelsInModelSystem(vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle, const r2::draw::ModelSystem& modelSystem, const r2::SArray<vb::GPUModelRefHandle>* handles)
	{
		return false;
	}

	bool UnloadAllModelsInModelSystem(vb::VertexBufferSystem& system, const vb::VertexBufferHandle& handle, const ModelSystem& modelSystem)
	{
		return false;
	}
}