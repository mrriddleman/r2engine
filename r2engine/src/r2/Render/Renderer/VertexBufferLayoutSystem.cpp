#include "r2pch.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Renderer/RenderKey.h"

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

	vb::VertexBufferLayoutSystem* CreateVertexBufferSystem(const r2::mem::MemoryArea::Handle memoryAreaHandle)
	{
		return nullptr;
	}

	void FreeVertexBufferSystem(vb::VertexBufferLayoutSystem* system)
	{

	}

	u64 VertexBufferLayoutSystem::MemorySize(u32 numBufferLayouts, u32 maxModelsLoaded, u32 avgNumberOfMeshesPerModel, u64 alignment)
	{
		return 0;
	}


}

namespace r2::draw::vbsys
{
	vb::VertexBufferLayoutHandle AddVertexBufferLayout(vb::VertexBufferLayoutSystem& system, const r2::draw::BufferLayoutConfiguration& vertexConfig)
	{
		return {};
	}

	vb::VertexBufferLayoutSize GetVertexBufferCapacity(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		return {};
	}

	vb::VertexBufferLayoutSize GetVertexBufferSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		return {};
	}

	vb::VertexBufferLayoutSize GetVertexBufferRemainingSize(const vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle)
	{
		return {};
	}

	//@TODO(Serge): Figure out how this will interact with the renderer - ie. will this generate the command to upload the model data or will this call the renderer to do that etc.
	//				or will this just update some internal data based on the model data
	//				Remember the buffer should resize if we're beyond the size
	vb::GPUModelRefHandle UploadModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::Model& model)
	{
		return {};
	}

	vb::GPUModelRefHandle UploadAnimModelToVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::VertexBufferLayoutHandle& handle, const r2::draw::AnimModel& model)
	{
		return {};
	}

	//Somehow the modelRefHandle will be used to figure out which vb::VertexBufferHandle is being used for it
	bool UnloadModelFromVertexBuffer(vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& modelRefHandle)
	{
		return false;
	}

	bool IsModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::Model& model)
	{
		return false;
	}

	bool IsAnimModelLoaded(const vb::VertexBufferLayoutSystem& system, const r2::draw::AnimModel& model)
	{
		return false;
	}


	//Should return true if the model is still loaded - false otherwise
	bool IsModelRefHandleValid(const vb::VertexBufferLayoutSystem& system, const vb::GPUModelRefHandle& handle)
	{
		return false;
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