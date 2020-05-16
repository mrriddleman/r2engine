#include "r2pch.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Model/Mesh.h"

namespace r2::draw::cmd
{
	const r2::draw::dispatch::BackendDispatchFunction Clear::DispatchFunc = &r2::draw::dispatch::Clear;
	const r2::draw::dispatch::BackendDispatchFunction DrawIndexed::DispatchFunc = &r2::draw::dispatch::DrawIndexed;
	const r2::draw::dispatch::BackendDispatchFunction FillVertexBuffer::DispatchFunc = &r2::draw::dispatch::FillVertexBuffer;
	const r2::draw::dispatch::BackendDispatchFunction FillIndexBuffer::DispatchFunc = &r2::draw::dispatch::FillIndexBuffer;

	u64 LargestCommand()
	{
		return std::max({
			sizeof(r2::draw::cmd::Clear),
			sizeof(r2::draw::cmd::DrawIndexed),
			sizeof(r2::draw::cmd::FillVertexBuffer),
			sizeof(r2::draw::cmd::FillIndexBuffer)
			});
	}

	u64 FillVertexBufferCommand(FillVertexBuffer* cmd, const Mesh& mesh, VertexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr )
		{
			R2_CHECK(false, "cmd or model is null");
			return 0;
		}

		const u64 numVertices = r2::sarr::Size(*mesh.optrVertices);

		cmd->vertexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof(r2::sarr::At(*mesh.optrVertices, 0)) * numVertices;
		cmd->data = r2::sarr::Begin(*mesh.optrVertices);

		return cmd->dataSize + offset;
	}

	u64 FillIndexBufferCommand(FillIndexBuffer* cmd, const Mesh& mesh, IndexBufferHandle handle, u64 offset)
	{
		if (cmd == nullptr)
		{
			R2_CHECK(false, "cmd or model is null");
			return  0;
		}

		const u64 numIndices = r2::sarr::Size(*mesh.optrIndices);

		cmd->indexBufferHandle = handle;
		cmd->offset = offset;
		cmd->dataSize = sizeof( r2::sarr::At(*mesh.optrIndices, 0) ) * numIndices;
		cmd->data = r2::sarr::Begin(*mesh.optrIndices);

		return cmd->dataSize + offset;
	}
}