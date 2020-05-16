#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/BackendDispatch.h"


namespace r2::draw
{
	struct Mesh;
}

namespace r2::draw::cmd
{

	extern u32 CLEAR_COLOR_BUFFER;

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
		void* data;
		u64 dataSize;
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


	//hmm
	struct Batch
	{

	};

	u64 LargestCommand();
	u64 FillVertexBufferCommand(FillVertexBuffer* cmd, const Mesh& mesh, VertexBufferHandle handle, u64 offset = 0);
	u64 FillIndexBufferCommand(FillIndexBuffer* cmd, const Mesh& mesh, IndexBufferHandle handle, u64 offset = 0);
}

#endif