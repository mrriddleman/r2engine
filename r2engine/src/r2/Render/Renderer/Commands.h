#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BackendDispatch.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/Shader.h"

namespace r2::draw
{
	struct Mesh;
}

//@TODO(Serge): change these to packets
//Commands will be for groups of packets

namespace r2::draw::cmd
{
	extern u32 CLEAR_COLOR_BUFFER;
	extern u32 CLEAR_DEPTH_BUFFER;
	extern u32 CULL_FACE_FRONT;
	extern u32 CULL_FACE_BACK;

	extern u32 SHADER_STORAGE_BARRIER_BIT;
	extern u32 FRAMEBUFFER_BARRIER_BIT;
	extern u32 ALL_BARRIER_BITS;
	

	struct DrawState
	{
		DrawLayer layer;
		b32 depthEnabled; //@TODO(Serge): change this to flags (u32)
		u32 cullState;
	};

	struct Clear
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 flags;
	};
	static_assert(std::is_pod<Clear>::value == true, "Clear must be a POD.");

	struct DrawIndexed
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 indexCount;
		u32 startIndex;
		u32 baseVertex;

		//need some kind of layout (VAO - OpenGL) 
		r2::draw::BufferLayoutHandle bufferLayoutHandle;
		r2::draw::VertexBufferHandle vertexBufferHandle;
		r2::draw::IndexBufferHandle indexBufferHandle;
	};
	static_assert(std::is_pod<DrawIndexed>::value == true, "DrawIndexed must be a POD.");

	struct FillVertexBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::VertexBufferHandle vertexBufferHandle;

		u64 offset;
		u64 dataSize;
		void* data;
	};
	static_assert(std::is_pod<FillVertexBuffer>::value == true, "FillVertexBuffer must be a POD.");

	struct FillIndexBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::IndexBufferHandle indexBufferHandle;

		u64 offset;
		u64 dataSize;
		void* data;
	};
	static_assert(std::is_pod<FillIndexBuffer>::value == true, "FillIndexBuffer must be a POD.");


	struct FillConstantBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::ConstantBufferHandle constantBufferHandle;
		r2::draw::ConstantBufferLayout::Type type;
		b32 isPersistent;
		u64 offset;
		u64 dataSize;
		void* data;
	};
	static_assert(std::is_pod<FillConstantBuffer>::value == true, "FillContantBuffer must be a POD.");
	
	struct CompleteConstantBuffer
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		r2::draw::ConstantBufferHandle constantBufferHandle;
		u64 count;
	};
	static_assert(std::is_pod<CompleteConstantBuffer>::value == true, "CompleteConstantBuffer must be a POD.");


	struct DrawBatchSubCommand
	{
		u32  count;
		u32  instanceCount;
		u32  firstIndex;
		u32  baseVertex;
		u32  baseInstance;
	};

	struct DrawBatch
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		PrimitiveType primitiveType;
		DrawState state;
		r2::draw::BufferLayoutHandle bufferLayoutHandle;
		r2::draw::ConstantBufferHandle batchHandle;

		u32 startCommandIndex;
		u32 numSubCommands;
		DrawBatchSubCommand* subCommands;
	};
	static_assert(std::is_pod<DrawBatch>::value == true, "DrawBatch must be a POD.");

	struct DrawDebugBatchSubCommand
	{
		u32  count;
		u32  instanceCount;
		u32  firstVertex;
		u32  baseInstance;
	};

	struct DrawDebugBatch
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		PrimitiveType primitiveType;
		DrawState state;
		r2::draw::BufferLayoutHandle bufferLayoutHandle;
		r2::draw::ConstantBufferHandle batchHandle;
		u32 startCommandIndex;
		u32 numSubCommands;
		DrawDebugBatchSubCommand* subCommands;
	};
	static_assert(std::is_pod<DrawDebugBatch>::value == true, "DrawDebugBatch must be a POD.");

	struct SetRenderTarget
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 framebufferID;
		u32 width;
		u32 height;
		u32 xOffset;
		u32 yOffset;
		u32 numColorAttachments;
		u32 numDepthAttachments;
		u32 depthTexture;
	};
	static_assert(std::is_pod<SetRenderTarget>::value == true, "SetRenderTarget must be a POD.");

	//Compute stuff

	struct DispatchSubCommand
	{
		u32 numGroupsX;
		u32 numGroupsY;
		u32 numGroupsZ;
	};

	struct DispatchComputeIndirect
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		r2::draw::ConstantBufferHandle batchHandle;
		u32 offset;
	};

	static_assert(std::is_pod<DispatchComputeIndirect>::value == true, "DispatchComputeIndirect must be a POD.");


	struct DispatchCompute
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;

		u32 numGroupsX;
		u32 numGroupsY;
		u32 numGroupsZ;
	};

	static_assert(std::is_pod<DispatchCompute>::value == true, "DispatchCompute must be a POD.");


	struct Barrier
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		u32 flags;
	};

	static_assert(std::is_pod<Barrier>::value == true, "Barrier must be a POD.");

	const u32 MAX_NUM_CHARACTERS_IN_UNIFORM_NAME = 32;
	
	struct ConstantUint
	{
		static const r2::draw::dispatch::BackendDispatchFunction DispatchFunc;
		char uniformName[MAX_NUM_CHARACTERS_IN_UNIFORM_NAME + 1];
		ShaderHandle shaderID;
		u32 value;
	};

	static_assert(std::is_pod<ConstantUint>::value == true, "ConstantUint must be a POD.");


	u64 LargestCommand();
}

#endif